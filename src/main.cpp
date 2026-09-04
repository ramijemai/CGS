#include "Services/CapacityEngine.h"
#include "Services/MissionPlanner.h"
#include "Services/RecoveryService.h"
#include "Services/TelemetryManager.h"
#include "Services/MissionRepository.h"
#include "Services/MavlinkFlightController.h"
#include "Controller/MissionController.h"
#include "Controller/TelemetryWebSocketController.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

void runSession(tcp::socket socket, MissionController& missionCtrl, TelemetryWebSocketController& wsCtrl) {
    beast::error_code ec;
    beast::flat_buffer buffer;

    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if (ec) return;

    if (websocket::is_upgrade(req)) {
        if (req.target() == "/ws/telemetry") {
            websocket::stream<tcp::socket> ws(std::move(socket));
            ws.accept(req, ec);
            if (ec) return;

            while (true) {
                beast::flat_buffer wsBuffer;
                ws.read(wsBuffer, ec);
                if (ec == websocket::error::closed) break;
                if (ec) return;

                std::string message = beast::buffers_to_string(wsBuffer.data());
                wsCtrl.onMessage(message, [&ws](const std::string& reply) {
                    ws.text(true);
                    ws.write(asio::buffer(reply));
                });
            }
        }
        return;
    }

    http::response<http::string_body> res;
    res.version(req.version());
    res.keep_alive(false);
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "http://localhost:5173");
    res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
    res.set(http::field::access_control_allow_headers, "Content-Type");

    if (req.method() == http::verb::options) {
        res.result(http::status::no_content);
        res.prepare_payload();
        http::write(socket, res, ec);
        return;
    }

    if (req.method() == http::verb::get && req.target() == "/api/v1/bunker/status") {
        auto [code, body] = missionCtrl.handleGetBunkerStatus();
        res.result(code);
        res.body() = boost::json::serialize(body);
    }
    else if (req.method() == http::verb::get && req.target() == "/api/v1/missions/active") {
        auto [code, body] = missionCtrl.handleGetActiveMissions();
        res.result(code);
        res.body() = boost::json::serialize(body);
    }
    else if (req.method() == http::verb::post && req.target() == "/api/v1/missions/launch") {
        auto [code, body] = missionCtrl.handleLaunchMission(req.body());
        res.result(code);
        res.body() = boost::json::serialize(body);
    }
    else if (req.method() == http::verb::get && req.target() == "/api/v1/missions/history") {
        auto [code, body] = missionCtrl.handleGetMissionHistory();
        res.result(code);
        res.body() = boost::json::serialize(body);
    }
    else if (req.method() == http::verb::post && req.target() == "/api/v1/missions/launch-scan") {
        auto [code, body] = missionCtrl.handleLaunchAreaScan(req.body());
        res.result(code);
        res.body() = boost::json::serialize(body);
    }
    else {
        res.result(http::status::not_found);
        res.body() = R"({"error": "Endpoint not found"})";
    }

    res.prepare_payload();
    http::write(socket, res, ec);
}

int main() {
    try {
        Bunker bunker({36.822778, 10.203199, 0.0});

        // Single bay — the fleet is exactly one real, SITL-backed drone.
        // Increase this later if you add more physical/simulated drones.
        CapacityEngine bunkerEngine(1);

        auto drone = std::make_shared<Drone>("PV-SCOUT-01", 100.0);
        drone->setCurrentLocation(bunker.getGpsLocation());


        TelemetryManager telemetryManager;
        MissionRepository missionRepository("cgs_missions.db");
        MissionPlanner missionPlanner(bunkerEngine, telemetryManager, missionRepository);
        RecoveryService recoveryService(bunkerEngine);

        auto mavlinkController = std::make_unique<MavlinkFlightController>(
            missionPlanner, telemetryManager, recoveryService, bunker, drone
        );

        bool sitlConnected = mavlinkController->connect("udpin://0.0.0.0:14540");

if (sitlConnected) {
    if (mavlinkController->isOnGround()) {
    bunkerEngine.getSlot(1)->dockDrone(drone, false);
    std::cout << "[MAIN] '" << drone->getId() << "' confirmed on ground — docked in Bay 1.\n";
} else {
        std::cerr << "[MAIN] WARNING: '" << drone->getId()
                  << "' is already airborne on connect (leftover flight from a previous "
                  << "session?). Bay 1 left VACANT so recovery can dock it once it actually "
                  << "lands. Registering it as active telemetry now.\n";
        telemetryManager.registerActiveDrone(drone, drone->getCurrentLocation());

        // Kick off Phase 2 recovery: RTL + auto-dock through RecoveryService.
        std::cout << "[MAIN] Triggering RecoveryService::executeRecoveryAndDocking for '"
                  << drone->getId() << "' on startup.\n";
        recoveryService.executeRecoveryAndDocking(drone, bunker);
    }
    mavlinkController->start();
} else {
    // No telemetry available to verify reality against — fall back to the
    // old blind-dock behavior, since there's nothing else to check.
    bunkerEngine.getSlot(1)->dockDrone(drone);
    std::cerr << "[MAIN] WARNING: SITL not reachable on udpin://0.0.0.0:14540. '"
              << drone->getId() << "' docked without verification — start sim_vehicle.py "
              << "with --out=udp:127.0.0.1:14540 and restart this server to enable flight.\n";
}

        MissionController missionController(missionPlanner, bunkerEngine, missionRepository);
        TelemetryWebSocketController wsController(
            telemetryManager, recoveryService, bunkerEngine, bunker, missionPlanner,
            sitlConnected ? mavlinkController.get() : nullptr
        );

        asio::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 18080));

        std::cout << "[SERVER] Boost.Beast HTTP & WebSocket Server running on port 18080...\n";

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);

            std::thread([s = std::move(socket), &missionController, &wsController]() mutable {
                runSession(std::move(s), missionController, wsController);
            }).detach();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[CRITICAL] Server Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
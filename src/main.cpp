#include "Services/CapacityEngine.h"
#include "Services/MissionPlanner.h"
#include "Services/RecoveryService.h"
#include "Services/TelemetryManager.h"
#include "Controller/MissionController.h"
#include "Controller/TelemetryWebSocketController.h"
#include "Controller/DroneSimulator.h"
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

// Session handler for Boost.Beast
void runSession(tcp::socket socket, MissionController& missionCtrl, TelemetryWebSocketController& wsCtrl) {
    beast::error_code ec;
    beast::flat_buffer buffer;

    // 1. Read the HTTP Request
    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if (ec) return;

    // Check if it is a WebSocket upgrade request
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

// 2. Process REST Endpoints
    http::response<http::string_body> res;
    res.version(req.version());
    res.keep_alive(false);
    res.set(http::field::content_type, "application/json");

    // CORS headers — required for the Vite dev server (localhost:5173) to
    // read responses at all. Applied to every REST response, including errors.
    res.set(http::field::access_control_allow_origin, "http://localhost:5173");
    res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
    res.set(http::field::access_control_allow_headers, "Content-Type");

    // Handle CORS preflight requests explicitly — browsers send these
    // automatically before POST requests with a JSON content type.
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
     /*   Bunker bunker({36.773442, 10.285913, 0.0});

        // Initialize Core Services
        CapacityEngine bunkerEngine(3);
        auto droneAlpha = std::make_shared<Drone>("DRONE-ALPHA", 100.0);
        auto droneBeta  = std::make_shared<Drone>("DRONE-BETA", 95.0);
        auto droneTeta  = std::make_shared<Drone>("DRONE-TETA", 15.0);
        droneAlpha->setCurrentLocation(bunker.getGpsLocation());
        droneBeta->setCurrentLocation(bunker.getGpsLocation());
        droneTeta->setCurrentLocation(bunker.getGpsLocation());
        //make the drone defected : 
        bunkerEngine.getSlot(1)->dockDrone(droneAlpha);
        bunkerEngine.getSlot(2)->dockDrone(droneBeta);
        bunkerEngine.getSlot(3)->dockDrone(droneTeta);
        droneTeta->setState(DroneState::Fault);

*/

        Bunker bunker({36.773442, 10.285913, 0.0});

        CapacityEngine bunkerEngine(3);
        auto droneAlpha = std::make_shared<Drone>("DRONE-ALPHA", 100.0);
        auto droneBeta  = std::make_shared<Drone>("DRONE-BETA", 95.0);
        droneAlpha->setCurrentLocation(bunker.getGpsLocation());
        droneBeta->setCurrentLocation(bunker.getGpsLocation());
        bunkerEngine.getSlot(1)->dockDrone(droneAlpha);
        bunkerEngine.getSlot(2)->dockDrone(droneBeta);

        TelemetryManager telemetryManager;
        MissionPlanner missionPlanner(bunkerEngine, telemetryManager);

        // Initialize and start the dynamic drone movement simulator in the background
        DroneSimulator simulator(missionPlanner, telemetryManager, droneAlpha, 15.0);
        DroneSimulator simulator2(missionPlanner, telemetryManager, droneBeta, 10.0);
        
        simulator.start();
        simulator2.start();


       // TelemetryManager telemetryManager;
      //  MissionPlanner missionPlanner(bunkerEngine, telemetryManager);
        RecoveryService recoveryService(bunkerEngine);

        // Instantiate Controllers
        MissionController missionController(missionPlanner, bunkerEngine);
        TelemetryWebSocketController wsController(telemetryManager, recoveryService, bunkerEngine, bunker, missionPlanner);        
        asio::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 18080));

        std::cout << "[SERVER] Boost.Beast HTTP & WebSocket Server running on port 18080...\n";

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);

            // Handle each client session in a detached thread
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
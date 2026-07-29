#include "Services/CapacityEngine.h"
#include "Services/MissionPlanner.h"
#include "Services/RecoveryService.h"
#include "Services/TelemetryManager.h"
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

    if (req.method() == http::verb::get && req.target() == "/api/v1/bunker/status") {
        auto [code, body] = missionCtrl.handleGetBunkerStatus();
        res.result(code);
        res.body() = boost::json::serialize(body);
    } 
    else if (req.method() == http::verb::post && req.target() == "/api/v1/missions/launch") {
        auto [code, body] = missionCtrl.handleLaunchMission(req.body());
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
        // Initialize Core Services
        CapacityEngine bunkerEngine(2);
        TelemetryManager telemetryManager;
        MissionPlanner missionPlanner(bunkerEngine);
        RecoveryService recoveryService(bunkerEngine);

        // Instantiate Controllers
        MissionController missionController(missionPlanner, bunkerEngine);
        TelemetryWebSocketController wsController(telemetryManager, recoveryService);

        // Boost.Asio Listener Setup
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
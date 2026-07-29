#include "Controller/TelemetryWebSocketController.h"
#include <exception>

namespace bj = boost::json;

TelemetryWebSocketController::TelemetryWebSocketController(TelemetryManager& telemetry, RecoveryService& recovery)
    : telemetryManager_(telemetry), recoveryService_(recovery) {}

void TelemetryWebSocketController::onMessage(const std::string& rawPayload, SendTextCallback sendReply) {
    try {
        bj::value parsed = bj::parse(rawPayload);
        const auto& obj = parsed.as_object();
        std::string action = bj::value_to<std::string>(obj.at("action"));

        if (action == "SEND_COMMAND") {
            TelemetryMessage cmd;
            cmd.droneId = bj::value_to<std::string>(obj.at("droneId"));
            cmd.command = static_cast<DroneCommand>(obj.at("commandCode").as_int64());
            
            if (obj.contains("text")) {
                cmd.messageText = bj::value_to<std::string>(obj.at("text"));
            } else {
                cmd.messageText = "Command dispatched via WS";
            }

            telemetryManager_.sendCommand(cmd.droneId, cmd);
            
            bj::object response{
                {"event", "COMMAND_ACK"},
                {"droneId", cmd.droneId},
                {"status", "DISPATCHED"}
            };
            sendReply(bj::serialize(response));
        }
    } catch (const std::exception& e) {
        bj::object errResponse{
            {"event", "ERROR"},
            {"message", e.what()}
        };
        sendReply(bj::serialize(errResponse));
    }
}

void TelemetryWebSocketController::broadcastTelemetry(SendTextCallback broadcast) {
    bj::object telemetryPacket{
        {"event", "TELEMETRY_UPDATE"}
    };
    broadcast(bj::serialize(telemetryPacket));
}
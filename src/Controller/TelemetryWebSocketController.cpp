#include "Controller/TelemetryWebSocketController.h"
#include "Common/DroneStateUtils.h"
#include <exception>

namespace bj = boost::json;

namespace {
    // Adjust these to match the real bounds of your DroneCommand enum.
    constexpr int64_t kMinCommandCode = 0;
    constexpr int64_t kMaxCommandCode = 4;

}

TelemetryWebSocketController::TelemetryWebSocketController(TelemetryManager& telemetry,
                                                             RecoveryService& recovery,
                                                             CapacityEngine& capacityEngine,
                                                             const Bunker& bunker,
                                                             MissionPlanner& missionPlanner,
                                                             MavlinkFlightController* mavlinkController)
    : telemetryManager_(telemetry)
    , recoveryService_(recovery)
    , capacityEngine_(capacityEngine)
    , bunker_(bunker)
    , missionPlanner_(missionPlanner)
    , mavlinkController_(mavlinkController) {}

std::shared_ptr<Drone> TelemetryWebSocketController::findDroneById(const std::string& droneId) const {
    for (const auto& slot : capacityEngine_.getAllSlots()) {
        if (auto drone = slot->getDroneIfOccupied(); drone && drone->getId() == droneId) {
            return drone;
        }
    }
    return telemetryManager_.getActiveDronePtr(droneId);
}

void TelemetryWebSocketController::onMessage(const std::string& rawPayload, SendTextCallback sendReply) {
    try {
        bj::value parsed = bj::parse(rawPayload);
        const auto& obj = parsed.as_object();
        std::string action = bj::value_to<std::string>(obj.at("action"));

        if (action == "SEND_COMMAND") {
            std::string droneId = bj::value_to<std::string>(obj.at("droneId"));
            int64_t rawCode = obj.at("commandCode").as_int64();

            if (rawCode < kMinCommandCode || rawCode > kMaxCommandCode) {
                bj::object err{
                    {"event", "ERROR"},
                    {"message", "commandCode out of valid range: " + std::to_string(rawCode)}
                };
                sendReply(bj::serialize(err));
                return;
            }

            TelemetryMessage cmd;
            cmd.droneId = droneId;
            cmd.command = static_cast<DroneCommand>(rawCode);
            cmd.messageText = obj.contains("text")
                ? bj::value_to<std::string>(obj.at("text"))
                : "Command dispatched via WS";

            telemetryManager_.sendCommand(cmd.droneId, cmd);

            bj::object response{
                {"event", "COMMAND_ACK"},
                {"droneId", cmd.droneId},
                {"status", "DISPATCHED"}
            };
            sendReply(bj::serialize(response));

        } else if (action == "INITIATE_RECOVERY") {
            std::string droneId = bj::value_to<std::string>(obj.at("droneId"));

            if (mavlinkController_ && mavlinkController_->ownsDrone(droneId)) {
                bool commanded = mavlinkController_->requestReturnToLaunch();
                bj::object response{
                    {"event", "RECOVERY_RESULT"},
                    {"droneId", droneId},
                    {"status", commanded ? "RTL_COMMANDED" : "FAILED"}
                };
                sendReply(bj::serialize(response));
                return;
            }

            auto drone = findDroneById(droneId);
            if (!drone) {
                bj::object err{
                    {"event", "ERROR"},
                    {"message", "Drone '" + droneId + "' not resolvable for recovery "
                                 "(not docked, and no active telemetry registration)."}
                };
                sendReply(bj::serialize(err));
                return;
            }

            bool ok = recoveryService_.executeRecoveryAndDocking(drone, bunker_);

            if (ok) {
                telemetryManager_.unregisterActiveDrone(droneId);
                missionPlanner_.completeMission(droneId, "COMPLETED");
            }

            bj::object response{
                {"event", "RECOVERY_RESULT"},
                {"droneId", droneId},
                {"status", ok ? "DOCKED" : "FAILED"}
            };
            sendReply(bj::serialize(response));

        } else if (action == "REQUEST_TELEMETRY") {
            broadcastTelemetry(sendReply);

        } else {
            bj::object response{
                {"event", "ERROR"},
                {"message", "Unknown action: " + action}
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
    bj::array dronesArray;

    for (const auto& telem : telemetryManager_.getAllActiveDronesTelemetry()) {
        dronesArray.push_back(bj::object{
            {"droneId", telem.droneId},
            {"latitude", telem.latitude},
            {"longitude", telem.longitude},
            {"altitude", telem.altitude},
            {"batteryLevel", telem.batteryLevel},
            {"speed", telem.speed},
            {"heading", telem.heading},
            {"state", DroneStateUtils::toString(telem.state)}
        });
    }

    bj::object telemetryPacket{
        {"event", "TELEMETRY_UPDATE"},
        {"drones", dronesArray}
    };
    broadcast(bj::serialize(telemetryPacket));
}
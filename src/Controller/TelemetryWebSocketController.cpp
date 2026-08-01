#include "Controller/TelemetryWebSocketController.h"
#include <exception>

namespace bj = boost::json;

namespace {
    // Adjust these to match the real bounds of your DroneCommand enum.
    constexpr int64_t kMinCommandCode = 0;
    constexpr int64_t kMaxCommandCode = 3;

    // Adjust to match the actual DroneState enum members in Domain/Drone.h
    std::string droneStateToString(DroneState state) {
        switch (state) {
            case DroneState::InFlight:          return "IN_FLIGHT";
            case DroneState::ReturningToBunker:  return "RETURNING";
            case DroneState::Landing:            return "LANDING";
            case DroneState::Fault:              return "FAULT";
            default:                              return "DOCKED";
        }
    }
}

TelemetryWebSocketController::TelemetryWebSocketController(TelemetryManager& telemetry,
                                                             RecoveryService& recovery,
                                                             CapacityEngine& capacityEngine,
                                                             const Bunker& bunker,
                                                             MissionPlanner& missionPlanner)
    : telemetryManager_(telemetry)
    , recoveryService_(recovery)
    , capacityEngine_(capacityEngine)
    , bunker_(bunker)
    , missionPlanner_(missionPlanner) {}

std::shared_ptr<Drone> TelemetryWebSocketController::findDroneById(const std::string& droneId) const {
    // Check docked bays first — covers drones that never launched, or that
    // have already been recovered and re-docked.
    for (const auto& slot : capacityEngine_.getAllSlots()) {
        if (slot->isOccupied() && slot->getDrone()->getId() == droneId) {
            return slot->getDrone();
        }
    }
    // Fall back to the active-telemetry registry — covers in-flight drones,
    // now that MissionPlanner registers them on dispatch.
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
                // Was missing: without this, a re-docked drone stays listed
                // as active telemetry forever.
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
            {"state", droneStateToString(telem.state)}
        });
    }

    bj::object telemetryPacket{
        {"event", "TELEMETRY_UPDATE"},
        {"drones", dronesArray}
    };
    broadcast(bj::serialize(telemetryPacket));
}
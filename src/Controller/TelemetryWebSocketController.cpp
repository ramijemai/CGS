#include "Controller/TelemetryWebSocketController.h"
#include <exception>

namespace bj = boost::json;

namespace {
    // Adjust these to match the real bounds of your DroneCommand enum.
    constexpr int64_t kMinCommandCode = 0;
    constexpr int64_t kMaxCommandCode = 3; // e.g. if RETURN_TO_BUNKER == 1 and it's not the last value
}

TelemetryWebSocketController::TelemetryWebSocketController(TelemetryManager& telemetry,
                                                             RecoveryService& recovery,
                                                             CapacityEngine& capacityEngine,
                                                             const Bunker& bunker)
    : telemetryManager_(telemetry)
    , recoveryService_(recovery)
    , capacityEngine_(capacityEngine)
    , bunker_(bunker) {}

std::shared_ptr<Drone> TelemetryWebSocketController::findDockedDroneById(const std::string& droneId) const {
    // LIMITATION: this only finds drones currently docked in a bay slot.
    // A drone that is mid-flight has been undocked (MissionPlanner calls
    // undockDrone() on dispatch) and its shared_ptr<Drone> isn't tracked
    // anywhere accessible right now — TelemetryManager only stores a copied
    // DroneTelemetry struct, not the original shared_ptr. So an
    // INITIATE_RECOVERY request for an in-flight drone will currently fail
    // to resolve here. See note below the code for the fix needed.
    for (const auto& slot : capacityEngine_.getAllSlots()) {
        if (slot->isOccupied() && slot->getDrone()->getId() == droneId) {
            return slot->getDrone();
        }
    }
    return nullptr;
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
            auto drone = findDockedDroneById(droneId);

            if (!drone) {
                bj::object err{
                    {"event", "ERROR"},
                    {"message", "Drone '" + droneId + "' not resolvable for recovery "
                                 "(not currently docked, or unknown ID)."}
                };
                sendReply(bj::serialize(err));
                return;
            }

            bool ok = recoveryService_.executeRecoveryAndDocking(drone, bunker_);

            bj::object response{
                {"event", "RECOVERY_RESULT"},
                {"droneId", droneId},
                {"status", ok ? "DOCKED" : "FAILED"}
            };
            sendReply(bj::serialize(response));

        } 
        
         else if (action == "REQUEST_TELEMETRY") {
    broadcastTelemetry(sendReply);
         }
        
        else {
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

namespace {
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
            {"state", droneStateToString(telem.state)}   // <-- new
        });
    }

    bj::object telemetryPacket{
        {"event", "TELEMETRY_UPDATE"},
        {"drones", dronesArray}
    };
    broadcast(bj::serialize(telemetryPacket));
}
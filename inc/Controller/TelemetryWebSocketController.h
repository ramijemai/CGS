#pragma once

#include "Services/TelemetryManager.h"
#include "Services/RecoveryService.h"
#include "Services/CapacityEngine.h"
#include "Domain/Bunker.h"
#include <boost/json.hpp>
#include <functional>
#include <string>

class TelemetryWebSocketController {
public:
    using SendTextCallback = std::function<void(const std::string&)>;

    // CapacityEngine& and Bunker& added so this controller can actually
    // resolve a drone by ID and call RecoveryService::executeRecoveryAndDocking,
    // which needs both a shared_ptr<Drone> and a Bunker reference.
    TelemetryWebSocketController(TelemetryManager& telemetry,
                                  RecoveryService& recovery,
                                  CapacityEngine& capacityEngine,
                                  const Bunker& bunker);

    void onMessage(const std::string& rawPayload, SendTextCallback sendReply);
    void broadcastTelemetry(SendTextCallback broadcast);

private:
    TelemetryManager& telemetryManager_;
    RecoveryService& recoveryService_;
    CapacityEngine& capacityEngine_;
    const Bunker& bunker_;

    std::shared_ptr<Drone> findDockedDroneById(const std::string& droneId) const;
};
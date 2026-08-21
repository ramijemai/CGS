#pragma once

#include "Services/TelemetryManager.h"
#include "Services/RecoveryService.h"
#include "Services/CapacityEngine.h"
#include "Services/MissionPlanner.h"
#include "Services/MavlinkFlightController.h"
#include "Domain/Bunker.h"
#include <boost/json.hpp>
#include <functional>
#include <string>

class TelemetryWebSocketController {
public:
    using SendTextCallback = std::function<void(const std::string&)>;

    TelemetryWebSocketController(TelemetryManager& telemetry,
                                  RecoveryService& recovery,
                                  CapacityEngine& capacityEngine,
                                  const Bunker& bunker,
                                  MissionPlanner& missionPlanner,
                                  MavlinkFlightController* mavlinkController = nullptr);

    void onMessage(const std::string& rawPayload, SendTextCallback sendReply);
    void broadcastTelemetry(SendTextCallback broadcast);

private:
    TelemetryManager& telemetryManager_;
    RecoveryService& recoveryService_;
    CapacityEngine& capacityEngine_;
    const Bunker& bunker_;
    MissionPlanner& missionPlanner_;
    MavlinkFlightController* mavlinkController_;

    std::shared_ptr<Drone> findDroneById(const std::string& droneId) const;
};
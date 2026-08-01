#pragma once
#include "Services/CapacityEngine.h"
#include "Services/TelemetryManager.h"
#include "Common/Types.h"
#include <ctime>
#include <string>
#include <vector>

struct ActiveMission {
    std::string missionId;
    std::string droneId;
    GpsCoordinate target;
    double cruiseAltitude{0.0};
    std::string status{"ACTIVE"};
    std::time_t launchTime{0};
};

class MissionPlanner {
public:
    MissionPlanner(CapacityEngine& capacityEngine, TelemetryManager& telemetryManager);

    bool planAndExecuteInspection(const GpsCoordinate& target,
                                  double cruiseAltitude,
                                  const std::string& requestedDroneId = "");

    // Moves the ACTIVE mission for this drone into history with the given
    // outcome. Returns false if no matching active mission was found.
    bool completeMission(const std::string& droneId, const std::string& outcome = "COMPLETED");

    const std::vector<ActiveMission>& getActiveMissions() const;
    const std::vector<ActiveMission>& getMissionHistory() const;

private:
    CapacityEngine& m_capacityEngine;
    TelemetryManager& m_telemetryManager;
    std::vector<ActiveMission> m_activeMissions;
    std::vector<ActiveMission> m_missionHistory;
    int m_nextMissionId{900};   // produces "M-900", "M-901", ... matching your mockup
};
#pragma once
#include "Services/CapacityEngine.h"
#include "Common/Types.h"
#include <string>
#include <vector>

struct ActiveMission {
    std::string droneId;
    GpsCoordinate target;
    double cruiseAltitude{0.0};
    std::string status{"ACTIVE"};
};

class MissionPlanner {
public:
    explicit MissionPlanner(CapacityEngine& capacityEngine);

    bool planAndExecuteInspection(const GpsCoordinate& target,
                                  double cruiseAltitude,
                                  const std::string& requestedDroneId = "");

    const std::vector<ActiveMission>& getActiveMissions() const;

private:
    CapacityEngine& m_capacityEngine;
    std::vector<ActiveMission> m_activeMissions;
};
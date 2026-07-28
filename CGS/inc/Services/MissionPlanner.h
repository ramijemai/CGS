#pragma once
#include "Services/CapacityEngine.h"
#include "Common/Types.h"

class MissionPlanner {
public:
    explicit MissionPlanner(CapacityEngine& capacityEngine);

    bool planAndExecuteInspection(const GpsCoordinate& target, double cruiseAltitude);

private:
    CapacityEngine& m_capacityEngine;
};
#pragma once

#include "Services/MissionPlanner.h"
#include "Services/CapacityEngine.h"
#include <boost/json.hpp>
#include <utility>
#include <string>

class MissionController {
public:
    MissionController(MissionPlanner& planner, CapacityEngine& capacityEngine);

    // Handles POST /api/v1/missions/launch
    std::pair<int, boost::json::value> handleLaunchMission(const std::string& requestBody);
    std::pair<int, boost::json::value> handleGetMissionHistory();

    // Handles GET /api/v1/bunker/status
    std::pair<int, boost::json::value> handleGetBunkerStatus();

    // Handles GET /api/v1/missions/active
    std::pair<int, boost::json::value> handleGetActiveMissions();

private:
    MissionPlanner& planner_;
    CapacityEngine& capacityEngine_;
};
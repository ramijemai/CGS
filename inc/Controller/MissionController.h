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

    // Handles GET /api/v1/bunker/status
    std::pair<int, boost::json::value> handleGetBunkerStatus();

    // Handles GET /api/v1/missions/active
    std::pair<int, boost::json::value> handleGetActiveMissions();

    // Handles GET /api/v1/missions/history
    std::pair<int, boost::json::value> handleGetMissionHistory();
    // Handles POST /api/v1/missions/area-scan
    std::pair<int, boost::json::value> handleLaunchAreaScan(const std::string& requestBody);

private:
    MissionPlanner& planner_;
    CapacityEngine& capacityEngine_;

    // Shared by handleGetActiveMissions/handleGetMissionHistory so both stay
    // in sync automatically — one place defines what a mission looks like
    // over the wire.
    boost::json::object serializeMission(const ActiveMission& mission) const;
};
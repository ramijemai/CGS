#pragma once

#include "Services/MissionPlanner.h"
#include "Services/CapacityEngine.h"
#include "Services/MissionRepository.h"
#include <boost/json.hpp>
#include <utility>
#include <string>

class MissionController {
public:
    MissionController(MissionPlanner& planner, CapacityEngine& capacityEngine, MissionRepository& missionRepository);

    std::pair<int, boost::json::value> handleLaunchMission(const std::string& requestBody);
    std::pair<int, boost::json::value> handleGetBunkerStatus();
    std::pair<int, boost::json::value> handleGetActiveMissions();
    std::pair<int, boost::json::value> handleGetMissionHistory();
    std::pair<int, boost::json::value> handleLaunchAreaScan(const std::string& requestBody);

private:
    MissionPlanner& planner_;
    CapacityEngine& capacityEngine_;
    MissionRepository& missionRepository_;

    boost::json::object serializeMission(const ActiveMission& mission) const;
    boost::json::object serializeStoredMission(const StoredMissionRecord& record) const;
};
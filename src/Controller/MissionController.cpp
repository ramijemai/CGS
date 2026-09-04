#include "Controller/MissionController.h"
#include "Common/DroneStateUtils.h"
#include "Common/MissionPatternUtils.h"
#include <exception>
#include <ctime>

namespace bj = boost::json;

namespace {
   

    std::string formatLaunchTime(std::time_t t) {
        char buf[6];
        std::tm tmv{};
        localtime_r(&t, &tmv);
        std::strftime(buf, sizeof(buf), "%H:%M", &tmv);
        return std::string(buf);
    }

}
    


MissionController::MissionController(MissionPlanner& planner, CapacityEngine& capacityEngine, MissionRepository& missionRepository)
    : planner_(planner), capacityEngine_(capacityEngine), missionRepository_(missionRepository) {}

std::pair<int, bj::value> MissionController::handleLaunchMission(const std::string& requestBody) {
    try {
        bj::value parsed = bj::parse(requestBody);
        const auto& obj = parsed.as_object();

        GpsCoordinate target{
            obj.at("latitude").to_number<double>(),
            obj.at("longitude").to_number<double>(),
            obj.at("altitude").to_number<double>()
        };
        double cruiseAltitude = obj.at("cruiseAltitude").to_number<double>();

        std::string requestedDroneId;
        if (obj.contains("droneId")) {
            requestedDroneId = bj::value_to<std::string>(obj.at("droneId"));
        }

        bool result = planner_.planAndExecuteInspection(target, cruiseAltitude, requestedDroneId);

        if (result) {
            return {200, bj::object{{"status", "SUCCESS"}, {"message", "Mission launched successfully."}}};
        } else {
            return {400, bj::object{{"status", "FAILED"}, {"message", "Launch requirements not met."}}};
        }
    } catch (const std::exception& e) {
        return {400, bj::object{{"status", "ERROR"}, {"message", std::string("Invalid payload: ") + e.what()}}};
    }
}

std::pair<int, bj::value> MissionController::handleGetBunkerStatus() {
    bj::array slotsArray;

for (const auto& slot : capacityEngine_.getAllSlots()) {
    bj::object slotJson;
    slotJson["slotId"] = slot->getSlotId();
    slotJson["isOccupied"] = slot->isOccupied();

    if (auto drone = slot->getDroneIfOccupied()) {
        slotJson["drone"] = bj::object{
            {"id", drone->getId()},
            {"batteryLevel", drone->getBatteryLevel()},
            {"state", DroneStateUtils::toString(drone->getState())}
        };
    }
    slotsArray.push_back(slotJson);
}

    return {200, bj::object{{"slots", slotsArray}}};
}

bj::object MissionController::serializeMission(const ActiveMission& mission) const {
    bj::array waypointsArray;
    for (const auto& wp : mission.waypoints) {
        waypointsArray.push_back(bj::object{
            {"latitude", wp.latitude},
            {"longitude", wp.longitude},
            {"altitude", wp.altitude}
        });
    }

    std::int64_t durationSeconds = 0;
if (mission.completionTime > 0 && mission.launchTime > 0) {
    durationSeconds = static_cast<std::int64_t>(mission.completionTime - mission.launchTime);
}


    return bj::object{
        {"missionId", mission.missionId},
        {"droneId", mission.droneId},
        {"status", mission.status},
        {"pattern", MissionPatternUtils::toString(mission.pattern)},
        {"target", bj::object{{"latitude", mission.target.latitude},
                               {"longitude", mission.target.longitude},
                               {"altitude", mission.target.altitude}}},
        {"waypoints", waypointsArray},
        {"currentWaypointIndex", static_cast<std::int64_t>(mission.currentWaypointIndex)},
        {"finalWaypointReached", mission.finalWaypointReached},
        {"cruiseAltitude", mission.cruiseAltitude},
        {"launchTime", formatLaunchTime(mission.launchTime)},
        {"durationSeconds", durationSeconds}  

    };
}

std::pair<int, bj::value> MissionController::handleGetActiveMissions() {
    bj::array missionsArray;
    for (const auto& mission : planner_.getActiveMissions()) {
        missionsArray.push_back(serializeMission(mission));
    }
    return {200, bj::object{{"missions", missionsArray}}};
}


std::pair<int, bj::value> MissionController::handleLaunchAreaScan(const std::string& requestBody) {
    try {
        bj::value parsed = bj::parse(requestBody);
        const auto& obj = parsed.as_object();

        if (!obj.contains("waypoints") || !obj.at("waypoints").is_array()) {
            return {400, bj::object{{"status", "ERROR"}, {"message", "waypoints must be an array"}}};
        }

        AreaScanRequest request;
        request.cruiseAltitude = obj.at("cruiseAltitude").to_number<double>();

        for (const auto& wpVal : obj.at("waypoints").as_array()) {
            const auto& wpObj = wpVal.as_object();
            request.waypoints.push_back(GpsCoordinate{
                wpObj.at("latitude").to_number<double>(),
                wpObj.at("longitude").to_number<double>(),
                0.0 // set uniformly from cruiseAltitude in MissionPlanner
            });
        }

        if (request.waypoints.size() < 2) {
            return {400, bj::object{{"status", "ERROR"}, {"message", "At least 2 waypoints are required"}}};
        }

        std::string requestedDroneId;
        if (obj.contains("droneId")) {
            requestedDroneId = bj::value_to<std::string>(obj.at("droneId"));
        }

        bool result = planner_.planAndExecuteAreaScan(request, requestedDroneId);

        if (result) {
            return {200, bj::object{{"status", "SUCCESS"}, {"message", "Waypoint path mission launched successfully."}}};
        } else {
            return {400, bj::object{{"status", "FAILED"}, {"message", "Launch requirements not met."}}};
        }
    } catch (const std::exception& e) {
        return {400, bj::object{{"status", "ERROR"}, {"message", std::string("Invalid payload: ") + e.what()}}};
    }
}

boost::json::object MissionController::serializeStoredMission(const StoredMissionRecord& record) const {
    bj::array waypointsArray;
    for (const auto& wp : record.waypoints) {
        waypointsArray.push_back(bj::object{
            {"latitude", wp.latitude},
            {"longitude", wp.longitude},
            {"altitude", wp.altitude}
        });
    }

    std::int64_t durationSeconds = 0;
    if (record.completionTime > 0 && record.launchTime > 0) {
        durationSeconds = static_cast<std::int64_t>(record.completionTime - record.launchTime);
    }

    return bj::object{
        {"missionId", record.missionId},
        {"droneId", record.droneId},
        {"status", record.status},
        {"pattern", record.pattern},
        {"target", waypointsArray.empty()
            ? bj::object{{"latitude", 0.0}, {"longitude", 0.0}, {"altitude", 0.0}}
            : bj::object{{"latitude", record.waypoints[0].latitude},
                         {"longitude", record.waypoints[0].longitude},
                         {"altitude", record.waypoints[0].altitude}}},
        {"waypoints", waypointsArray},
        {"currentWaypointIndex", 0},
        {"cruiseAltitude", record.cruiseAltitude},
        {"launchTime", formatLaunchTime(record.launchTime)},
        {"durationSeconds", durationSeconds}
    };
}

std::pair<int, bj::value> MissionController::handleGetMissionHistory() {
    bj::array missionsArray;
    // NEW: now reads from the persisted database, not in-memory state —
    // survives server restarts.
    for (const auto& record : missionRepository_.getMissionHistory()) {
        missionsArray.push_back(serializeStoredMission(record));
    }
    return {200, bj::object{{"missions", missionsArray}}};
}
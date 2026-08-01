#include "Controller/MissionController.h"
#include <exception>

namespace bj = boost::json;

MissionController::MissionController(MissionPlanner& planner, CapacityEngine& capacityEngine)
    : planner_(planner), capacityEngine_(capacityEngine) {}

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

        if (slot->isOccupied()) {
            const auto drone = slot->getDrone();
            slotJson["drone"] = bj::object{
                {"id", drone->getId()},
                {"batteryLevel", drone->getBatteryLevel()}
            };
        }
        slotsArray.push_back(slotJson);
    }

    return {200, bj::object{{"slots", slotsArray}}};
}



namespace {
    std::string formatLaunchTime(std::time_t t) {
        char buf[6];
        std::tm tmv{};
        localtime_r(&t, &tmv);
        std::strftime(buf, sizeof(buf), "%H:%M", &tmv);
        return std::string(buf);
    }
}

std::pair<int, bj::value> MissionController::handleGetActiveMissions() {
    bj::array missionsArray;

    for (const auto& mission : planner_.getActiveMissions()) {
        missionsArray.push_back(bj::object{
            {"missionId", mission.missionId},
            {"droneId", mission.droneId},
            {"status", mission.status},
            {"target", bj::object{{"latitude", mission.target.latitude},
                                   {"longitude", mission.target.longitude},
                                   {"altitude", mission.target.altitude}}},
            {"cruiseAltitude", mission.cruiseAltitude},
            {"launchTime", formatLaunchTime(mission.launchTime)}
        });
    }

    return {200, bj::object{{"missions", missionsArray}}};
}

std::pair<int, bj::value> MissionController::handleGetMissionHistory() {
    bj::array missionsArray;

    for (const auto& mission : planner_.getMissionHistory()) {
        missionsArray.push_back(bj::object{
            {"missionId", mission.missionId},
            {"droneId", mission.droneId},
            {"status", mission.status},
            {"target", bj::object{{"latitude", mission.target.latitude},
                                   {"longitude", mission.target.longitude},
                                   {"altitude", mission.target.altitude}}},
            {"cruiseAltitude", mission.cruiseAltitude},
            {"launchTime", formatLaunchTime(mission.launchTime)}
        });
    }

    return {200, bj::object{{"missions", missionsArray}}};
}
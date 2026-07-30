#include "Controller/MissionController.h"
#include <exception>

namespace bj = boost::json;

MissionController::MissionController(MissionPlanner& planner, CapacityEngine& capacityEngine)
    : planner_(planner), capacityEngine_(capacityEngine) {}

std::pair<int, bj::value> MissionController::handleLaunchMission(const std::string& requestBody) {
    try {
        bj::value parsed = bj::parse(requestBody);
        const auto& obj = parsed.as_object();

        // to_number<double>() accepts both "100" (int64) and "100.0" (double) from JSON.
        // as_double() would throw on the former, rejecting perfectly valid requests.
        GpsCoordinate target{
            obj.at("latitude").to_number<double>(),
            obj.at("longitude").to_number<double>(),
            obj.at("altitude").to_number<double>()
        };

        // NOTE: renamed from "durationSeconds" -> "cruiseAltitude" to match what
        // MissionPlanner::planAndExecuteInspection actually expects as its 2nd param.
        // The old field name was silently feeding a duration value in as an altitude.
        double cruiseAltitude = obj.at("cruiseAltitude").to_number<double>();

        bool result = planner_.planAndExecuteInspection(target, cruiseAltitude);

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
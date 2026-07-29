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
            obj.at("latitude").as_double(),
            obj.at("longitude").as_double(),
            obj.at("altitude").as_double()
        };
        double duration = obj.at("durationSeconds").as_double();

        bool result = planner_.planAndExecuteInspection(target, duration);

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
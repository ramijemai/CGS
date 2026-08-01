#include "Services/MissionPlanner.h"
#include <iostream>

const std::vector<ActiveMission>& MissionPlanner::getActiveMissions() const {
    return m_activeMissions;
}

MissionPlanner::MissionPlanner(CapacityEngine& capacityEngine) 
    : m_capacityEngine(capacityEngine) {}

bool MissionPlanner::planAndExecuteInspection(const GpsCoordinate& target,
                                               double cruiseAltitude,
                                               const std::string& requestedDroneId) {
    std::cout << "\n==================================================\n";
    std::cout << "        INITIATING PHASE 1: MISSION DISPATCH       \n";
    std::cout << "==================================================\n";

    std::shared_ptr<BaySlot> slot;
    if (!requestedDroneId.empty()) {
        slot = m_capacityEngine.findDroneSlotById(requestedDroneId);
        if (!slot) {
            std::cerr << "[MISSION PLANNER ERROR] Drone '" << requestedDroneId
                      << "' is not currently docked in a bunker slot.\n";
            return false;
        }

        if (!slot->isOccupied() || !slot->getDrone()->isReadyForMission()) {
            std::cerr << "[MISSION PLANNER ERROR] Drone '" << requestedDroneId
                      << "' is not ready for mission dispatch.\n";
            return false;
        }
    } else {
        slot = m_capacityEngine.findReadyDroneSlot();
    }

    if (!slot) {
        std::cerr << "[MISSION PLANNER ERROR] No ready drones available in any bunker slot.\n";
        return false;
    }

    // Undock the drone — takes ownership out of the slot
    auto drone = slot->undockDrone();
    if (!drone) {
        std::cerr << "[MISSION PLANNER ERROR] Failed to undock drone from Bay " << slot->getSlotId() << ".\n";
        return false;
    }

    std::cout << "[MISSION PLANNER] Selected Asset: '" << drone->getId() 
              << "' in Bay " << slot->getSlotId() 
              << " (Battery: " << drone->getBatteryLevel() << "%)\n";

    if (drone->getState() == DroneState::Fault) {
        std::cerr << "[MISSION PLANNER ERROR] Diagnostics failed: Drone in FAULT state.\n";
        return false;
    }

    std::cout << "[MISSION PLANNER] Pre-flight diagnostics PASSED. Systems NOMINAL.\n";

    drone->setState(DroneState::InFlight);
    m_activeMissions.push_back({drone->getId(), target, cruiseAltitude, "ACTIVE"});

    std::cout << "[MISSION PLANNER] Drone dispatched to Target GPS (" 
              << target.latitude << ", " << target.longitude 
              << ") at " << cruiseAltitude << "m.\n";
    std::cout << "[MISSION PLANNER] Bay " << slot->getSlotId() << " is now VACANT.\n";

    return true;
}

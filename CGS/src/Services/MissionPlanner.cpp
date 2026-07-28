#include "Services/MissionPlanner.h"
#include <iostream>

MissionPlanner::MissionPlanner(CapacityEngine& capacityEngine) 
    : m_capacityEngine(capacityEngine) {}

bool MissionPlanner::planAndExecuteInspection(const GpsCoordinate& target, double cruiseAltitude) {
    std::cout << "\n==================================================\n";
    std::cout << "        INITIATING PHASE 1: MISSION DISPATCH       \n";
    std::cout << "==================================================\n";

    auto slot = m_capacityEngine.findReadyDroneSlot();
    if (!slot) {
        std::cerr << "[MISSION PLANNER ERROR] No ready drones available in any bunker slot.\n";
        return false;
    }

    auto drone = slot->getDrone();
    std::cout << "[MISSION PLANNER] Selected Asset: '" << drone->getId() 
              << "' in Bay " << slot->getSlotId() 
              << " (Battery: " << drone->getBatteryLevel() << "%)\n";

    if (drone->getState() == DroneState::Fault) {
        std::cerr << "[MISSION PLANNER ERROR] Diagnostics failed: Drone in FAULT state.\n";
        return false;
    }

    std::cout << "[MISSION PLANNER] Pre-flight diagnostics PASSED. Systems NOMINAL.\n";

    drone->setState(DroneState::InFlight);
    m_capacityEngine.getSlot(slot->getSlotId())->undockDrone();

    std::cout << "[MISSION PLANNER] Drone dispatched to Target GPS (" 
              << target.latitude << ", " << target.longitude 
              << ") at " << cruiseAltitude << "m.\n";
    std::cout << "[MISSION PLANNER] Bay " << slot->getSlotId() << " is now VACANT.\n";

    return true;
}
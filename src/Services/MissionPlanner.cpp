#include "Services/MissionPlanner.h"
#include <iostream>
#include <algorithm>

MissionPlanner::MissionPlanner(CapacityEngine& capacityEngine, TelemetryManager& telemetryManager)
    : m_capacityEngine(capacityEngine), m_telemetryManager(telemetryManager) {}

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

    auto candidateDrone = slot->getDrone();
    if (!candidateDrone || candidateDrone->getState() == DroneState::Fault) {
        std::cerr << "[MISSION PLANNER ERROR] Diagnostics failed: Drone in FAULT state.\n";
        return false;
    }

    auto drone = slot->undockDrone();
    if (!drone) {
        std::cerr << "[MISSION PLANNER ERROR] Failed to undock drone from Bay " << slot->getSlotId() << ".\n";
        return false;
    }

    std::cout << "[MISSION PLANNER] Selected Asset: '" << drone->getId() 
              << "' in Bay " << slot->getSlotId() 
              << " (Battery: " << drone->getBatteryLevel() << "%)\n";
    std::cout << "[MISSION PLANNER] Pre-flight diagnostics PASSED. Systems NOMINAL.\n";

    drone->setState(DroneState::InFlight);

    ActiveMission mission;
    mission.missionId = "M-" + std::to_string(m_nextMissionId++);
    mission.droneId = drone->getId();
    mission.target = target;
    mission.cruiseAltitude = cruiseAltitude;
    mission.status = "ACTIVE";
    mission.launchTime = std::time(nullptr);
    m_activeMissions.push_back(mission);

    std::cout << "[MISSION PLANNER] Drone dispatched to Target GPS (" 
              << target.latitude << ", " << target.longitude 
              << ") at " << cruiseAltitude << "m.\n";
    std::cout << "[MISSION PLANNER] Bay " << slot->getSlotId() << " is now VACANT.\n";

    m_telemetryManager.registerActiveDrone(drone, drone->getCurrentLocation());

    return true;
}

bool MissionPlanner::completeMission(const std::string& droneId, const std::string& outcome) {
    auto it = std::find_if(m_activeMissions.begin(), m_activeMissions.end(),
        [&](const ActiveMission& m) { return m.droneId == droneId && m.status == "ACTIVE"; });

    if (it == m_activeMissions.end()) {
        return false;
    }

    it->status = outcome;
    m_missionHistory.push_back(*it);
    m_activeMissions.erase(it);
    return true;
}

const std::vector<ActiveMission>& MissionPlanner::getActiveMissions() const {
    return m_activeMissions;
}

const std::vector<ActiveMission>& MissionPlanner::getMissionHistory() const {
    return m_missionHistory;
}
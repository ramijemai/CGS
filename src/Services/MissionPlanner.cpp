#include "Services/MissionPlanner.h"
#include <iostream>
#include <algorithm>

MissionPlanner::MissionPlanner(CapacityEngine& capacityEngine, TelemetryManager& telemetryManager)
    : m_capacityEngine(capacityEngine), m_telemetryManager(telemetryManager) {}

std::shared_ptr<Drone> MissionPlanner::selectAndUndockDrone(const std::string& requestedDroneId) {
    std::shared_ptr<BaySlot> slot;

    if (!requestedDroneId.empty()) {
        slot = m_capacityEngine.findDroneSlotById(requestedDroneId);
        if (!slot) {
            std::cerr << "[MISSION PLANNER ERROR] Drone '" << requestedDroneId
                      << "' is not currently docked in a bunker slot.\n";
            return nullptr;
        }
        if (!slot->isOccupied() || !slot->getDrone()->isReadyForMission()) {
            std::cerr << "[MISSION PLANNER ERROR] Drone '" << requestedDroneId
                      << "' is not ready for mission dispatch.\n";
            return nullptr;
        }
    } else {
        slot = m_capacityEngine.findReadyDroneSlot();
    }

    if (!slot) {
        std::cerr << "[MISSION PLANNER ERROR] No ready drones available in any bunker slot.\n";
        return nullptr;
    }

    auto candidateDrone = slot->getDrone();
    if (!candidateDrone || candidateDrone->getState() == DroneState::Fault) {
        std::cerr << "[MISSION PLANNER ERROR] Diagnostics failed: Drone in FAULT state.\n";
        return nullptr;
    }

    auto drone = slot->undockDrone();
    if (!drone) {
        std::cerr << "[MISSION PLANNER ERROR] Failed to undock drone from Bay " << slot->getSlotId() << ".\n";
        return nullptr;
    }

    std::cout << "[MISSION PLANNER] Selected Asset: '" << drone->getId()
              << "' in Bay " << slot->getSlotId()
              << " (Battery: " << drone->getBatteryLevel() << "%)\n";
    std::cout << "[MISSION PLANNER] Pre-flight diagnostics PASSED. Systems NOMINAL.\n";

    return drone;
}

void MissionPlanner::finalizeDispatch(const std::shared_ptr<Drone>& drone, ActiveMission mission) {
    drone->setState(DroneState::InFlight);

    mission.missionId = "M-" + std::to_string(m_nextMissionId++);
    mission.droneId = drone->getId();
    mission.launchTime = std::time(nullptr);
    if (!mission.waypoints.empty()) {
        mission.target = mission.waypoints.front();
    }

    std::cout << "[MISSION PLANNER] Mission '" << mission.missionId << "' dispatched — "
              << mission.waypoints.size() << " waypoint(s), cruise altitude "
              << mission.cruiseAltitude << "m.\n";

    m_activeMissions.push_back(mission);
    m_telemetryManager.registerActiveDrone(drone, drone->getCurrentLocation());
}

bool MissionPlanner::advanceMissionWaypoint(const std::string& droneId) {
    auto it = std::find_if(m_activeMissions.begin(), m_activeMissions.end(),
                           [&](const ActiveMission& mission) {
                               return mission.droneId == droneId;
                           });

    if (it == m_activeMissions.end()) {
        return false;
    }

    if (it->waypoints.empty()) {
        return false;
    }

    if (it->currentWaypointIndex + 1 < it->waypoints.size()) {
        ++it->currentWaypointIndex;
        it->target = it->waypoints.at(it->currentWaypointIndex);
        return true;
    }

    return false;
}

bool MissionPlanner::completeMission(const std::string& droneId, const std::string& outcome) {
    auto it = std::find_if(m_activeMissions.begin(), m_activeMissions.end(),
                           [&](const ActiveMission& mission) {
                               return mission.droneId == droneId;
                           });

    if (it == m_activeMissions.end()) {
        return false;
    }

    ActiveMission completed = *it;
    completed.status = outcome;
    m_activeMissions.erase(it);
    m_missionHistory.push_back(completed);
    return true;
}

const std::vector<ActiveMission>& MissionPlanner::getActiveMissions() const {
    return m_activeMissions;
}

const std::vector<ActiveMission>& MissionPlanner::getMissionHistory() const {
    return m_missionHistory;
}

bool MissionPlanner::planAndExecuteInspection(const GpsCoordinate& target,
                                               double cruiseAltitude,
                                               const std::string& requestedDroneId) {
    std::cout << "\n==================================================\n";
    std::cout << "        INITIATING PHASE 1: MISSION DISPATCH       \n";
    std::cout << "==================================================\n";

    auto drone = selectAndUndockDrone(requestedDroneId);
    if (!drone) return false;

    ActiveMission mission;
    mission.pattern = MissionPattern::SINGLE_POINT;
    mission.target = target;
    mission.waypoints = { target };
    mission.cruiseAltitude = cruiseAltitude;
    mission.status = "ACTIVE";

    finalizeDispatch(drone, mission);
    return true;
}

bool MissionPlanner::planAndExecuteAreaScan(const AreaScanRequest& request,
                                             const std::string& requestedDroneId) {
    std::cout << "\n==================================================\n";
    std::cout << "      INITIATING PHASE 1: WAYPOINT PATH DISPATCH   \n";
    std::cout << "==================================================\n";

    if (request.waypoints.size() < 2) {
        std::cerr << "[MISSION PLANNER ERROR] Waypoint path requires at least 2 points.\n";
        return false;
    }

    std::vector<GpsCoordinate> waypoints = request.waypoints;
    for (auto& wp : waypoints) {
        wp.altitude = request.cruiseAltitude;
    }

    auto drone = selectAndUndockDrone(requestedDroneId);
    if (!drone) return false;

    ActiveMission mission;
    mission.pattern = MissionPattern::WAYPOINT_PATH;
    mission.waypoints = waypoints;
    mission.cruiseAltitude = request.cruiseAltitude;
    mission.status = "ACTIVE";

    finalizeDispatch(drone, mission);
    return true;
}
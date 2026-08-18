#include "Services/MissionPlanner.h"
#include "Common/MissionPatternUtils.h"
#include <iostream>
#include <algorithm>

MissionPlanner::MissionPlanner(CapacityEngine& capacityEngine,
                                TelemetryManager& telemetryManager,
                                MissionRepository& missionRepository)
    : m_capacityEngine(capacityEngine)
    , m_telemetryManager(telemetryManager)
    , m_missionRepository(missionRepository) {}

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

    mission.droneId = drone->getId();
    mission.launchTime = std::time(nullptr);
    if (!mission.waypoints.empty()) {
        mission.target = mission.waypoints.front();
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        mission.missionId = "M-" + std::to_string(m_nextMissionId++);
        m_activeMissions.push_back(mission);
    }

    std::cout << "[MISSION PLANNER] Mission '" << mission.missionId << "' dispatched — "
              << mission.waypoints.size() << " waypoint(s), cruise altitude "
              << mission.cruiseAltitude << "m.\n";

    m_telemetryManager.registerActiveDrone(drone, drone->getCurrentLocation());

    // NEW: persist the mission the instant it's dispatched.
    m_missionRepository.recordMissionLaunch(
        mission.missionId,
        mission.droneId,
        MissionPatternUtils::toString(mission.pattern),
        mission.waypoints,
        mission.cruiseAltitude,
        mission.launchTime
    );
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
    for (auto& wp : waypoints) wp.altitude = request.cruiseAltitude;

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

bool MissionPlanner::advanceMissionWaypoint(const std::string& droneId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_activeMissions.begin(), m_activeMissions.end(),
        [&](const ActiveMission& m) { return m.droneId == droneId && m.status == "ACTIVE"; });

    if (it == m_activeMissions.end()) return false;
    if (it->isFinalWaypoint()) return false;

    it->currentWaypointIndex++;
    return true;
}

bool MissionPlanner::completeMission(const std::string& droneId, const std::string& outcome) {
    std::string missionId;
    std::time_t completionTime = std::time(nullptr);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find_if(m_activeMissions.begin(), m_activeMissions.end(),
            [&](const ActiveMission& m) { return m.droneId == droneId && m.status == "ACTIVE"; });

        if (it == m_activeMissions.end()) return false;

        it->status = outcome;
        it->completionTime = completionTime;
        missionId = it->missionId;
        m_missionHistory.push_back(*it);
        m_activeMissions.erase(it);
    }

    // NEW: persist the outcome. Deliberately called after releasing the
    // lock above — the DB write doesn't need to hold up other threads
    // waiting on mission state.
    m_missionRepository.recordMissionCompletion(missionId, outcome, completionTime);

    return true;
}

std::vector<ActiveMission> MissionPlanner::getActiveMissions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeMissions;
}

std::vector<ActiveMission> MissionPlanner::getMissionHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_missionHistory;
}

bool MissionPlanner::getActiveMissionSnapshot(const std::string& droneId, ActiveMission& out) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_activeMissions.begin(), m_activeMissions.end(),
        [&](const ActiveMission& m) { return m.droneId == droneId && m.status == "ACTIVE"; });
    if (it == m_activeMissions.end()) return false;
    out = *it;
    return true;
}
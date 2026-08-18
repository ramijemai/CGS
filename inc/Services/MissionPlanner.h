#pragma once
#include "Services/CapacityEngine.h"
#include "Services/TelemetryManager.h"
#include "Services/MissionRepository.h"
#include "Common/Types.h"
#include "Common/MissionPattern.h"
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

struct ActiveMission {
    std::string missionId;
    std::string droneId;
    GpsCoordinate target;

    MissionPattern pattern{MissionPattern::SINGLE_POINT};
    std::vector<GpsCoordinate> waypoints;
    std::size_t currentWaypointIndex{0};

    double cruiseAltitude{0.0};
    std::string status{"ACTIVE"};
    std::time_t launchTime{0};
    std::time_t completionTime{0};

    const GpsCoordinate& currentTarget() const {
        return waypoints.empty() ? target : waypoints.at(currentWaypointIndex);
    }
    bool isFinalWaypoint() const {
        return waypoints.empty() || currentWaypointIndex + 1 >= waypoints.size();
    }
};

struct AreaScanRequest {
    std::vector<GpsCoordinate> waypoints;
    double cruiseAltitude{0.0};
};

class MissionPlanner {
public:
    MissionPlanner(CapacityEngine& capacityEngine,
                   TelemetryManager& telemetryManager,
                   MissionRepository& missionRepository);

    bool planAndExecuteInspection(const GpsCoordinate& target,
                                  double cruiseAltitude,
                                  const std::string& requestedDroneId = "");

    bool planAndExecuteAreaScan(const AreaScanRequest& request,
                                const std::string& requestedDroneId = "");

    bool advanceMissionWaypoint(const std::string& droneId);
    bool completeMission(const std::string& droneId, const std::string& outcome = "COMPLETED");

    std::vector<ActiveMission> getActiveMissions() const;
    std::vector<ActiveMission> getMissionHistory() const;
    bool getActiveMissionSnapshot(const std::string& droneId, ActiveMission& out) const;

private:
    CapacityEngine& m_capacityEngine;
    TelemetryManager& m_telemetryManager;
    MissionRepository& m_missionRepository;
    std::vector<ActiveMission> m_activeMissions;
    std::vector<ActiveMission> m_missionHistory;
    int m_nextMissionId{900};
    mutable std::mutex m_mutex;

    std::shared_ptr<Drone> selectAndUndockDrone(const std::string& requestedDroneId);
    void finalizeDispatch(const std::shared_ptr<Drone>& drone, ActiveMission mission);
};
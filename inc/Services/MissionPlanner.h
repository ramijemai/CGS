#pragma once
#include "Services/CapacityEngine.h"
#include "Services/TelemetryManager.h"
#include "Common/Types.h"
#include "Common/MissionPattern.h"
#include <ctime>
#include <string>
#include <vector>

struct ActiveMission {
    std::string missionId;
    std::string droneId;
    GpsCoordinate target;   // SINGLE_POINT: the only point. WAYPOINT_PATH: waypoints[0], kept in sync.

    MissionPattern pattern{MissionPattern::SINGLE_POINT};
    std::vector<GpsCoordinate> waypoints;
    std::size_t currentWaypointIndex{0};

    double cruiseAltitude{0.0};
    std::string status{"ACTIVE"};
    std::time_t launchTime{0};

    const GpsCoordinate& currentTarget() const {
        return waypoints.empty() ? target : waypoints.at(currentWaypointIndex);
    }
    bool isFinalWaypoint() const {
        return waypoints.empty() || currentWaypointIndex + 1 >= waypoints.size();
    }
};

// Explicit, client-supplied waypoint path — no server-side generation.
// The client (map click sequence) already produced the ordered points;
// cruiseAltitude is applied uniformly to all of them.
struct AreaScanRequest {
    std::vector<GpsCoordinate> waypoints;
    double cruiseAltitude{0.0};
};

class MissionPlanner {
public:
    MissionPlanner(CapacityEngine& capacityEngine, TelemetryManager& telemetryManager);

    bool planAndExecuteInspection(const GpsCoordinate& target,
                                  double cruiseAltitude,
                                  const std::string& requestedDroneId = "");

    bool planAndExecuteAreaScan(const AreaScanRequest& request,
                                const std::string& requestedDroneId = "");

    bool advanceMissionWaypoint(const std::string& droneId);
    bool completeMission(const std::string& droneId, const std::string& outcome = "COMPLETED");

    const std::vector<ActiveMission>& getActiveMissions() const;
    const std::vector<ActiveMission>& getMissionHistory() const;

private:
    CapacityEngine& m_capacityEngine;
    TelemetryManager& m_telemetryManager;
    std::vector<ActiveMission> m_activeMissions;
    std::vector<ActiveMission> m_missionHistory;
    int m_nextMissionId{900};

    std::shared_ptr<Drone> selectAndUndockDrone(const std::string& requestedDroneId);
    void finalizeDispatch(const std::shared_ptr<Drone>& drone, ActiveMission mission);
};
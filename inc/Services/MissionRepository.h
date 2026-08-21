#pragma once
#include "Common/Types.h"
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

struct sqlite3;

struct StoredMissionRecord {
    std::string missionId;
    std::string droneId;
    std::string pattern;
    double cruiseAltitude{0.0};
    std::string status;
    std::time_t launchTime{0};
    std::time_t completionTime{0};
    std::vector<GpsCoordinate> waypoints;
};

class MissionRepository {
public:
    explicit MissionRepository(const std::string& dbPath);
    ~MissionRepository();

    MissionRepository(const MissionRepository&) = delete;
    MissionRepository& operator=(const MissionRepository&) = delete;

    void recordMissionLaunch(const std::string& missionId,
                              const std::string& droneId,
                              const std::string& pattern,
                              const std::vector<GpsCoordinate>& waypoints,
                              double cruiseAltitude,
                              std::time_t launchTime);

    void recordMissionCompletion(const std::string& missionId,
                                  const std::string& outcome,
                                  std::time_t completionTime);

    // Returns every mission that has reached a terminal status (i.e. not
    // still ACTIVE), most recent launch first, each with its full
    // waypoint list attached.
    std::vector<StoredMissionRecord> getMissionHistory() const;

private:
    sqlite3* m_db{nullptr};
    mutable std::mutex m_mutex;

    void initSchema();
    void execOrThrow(const std::string& sql);
    std::vector<GpsCoordinate> loadWaypointsFor(const std::string& missionId) const;
};
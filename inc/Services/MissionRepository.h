#pragma once
#include "Common/Types.h"
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

struct sqlite3; // forward declaration — keeps <sqlite3.h> out of every file
                 // that transitively includes this header via MissionPlanner.h

class MissionRepository {
public:
    explicit MissionRepository(const std::string& dbPath);
    ~MissionRepository();

    MissionRepository(const MissionRepository&) = delete;
    MissionRepository& operator=(const MissionRepository&) = delete;

    // Persists a newly dispatched mission and its full waypoint list.
    // Best-effort: logs and returns on failure rather than throwing —
    // a DB write failure shouldn't prevent a real drone from launching.
    void recordMissionLaunch(const std::string& missionId,
                              const std::string& droneId,
                              const std::string& pattern,
                              const std::vector<GpsCoordinate>& waypoints,
                              double cruiseAltitude,
                              std::time_t launchTime);

    // Updates a mission's outcome once it completes, is aborted, or fails.
    void recordMissionCompletion(const std::string& missionId,
                                  const std::string& outcome,
                                  std::time_t completionTime);

private:
    sqlite3* m_db{nullptr};
    std::mutex m_mutex; // serializes all access — simpler and safer than
                         // relying on SQLite's own threading mode guarantees

    void initSchema();
    void execOrThrow(const std::string& sql);
};
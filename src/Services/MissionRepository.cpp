#include "Services/MissionRepository.h"
#include <sqlite3.h>
#include <iostream>
#include <stdexcept>

MissionRepository::MissionRepository(const std::string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &m_db) != SQLITE_OK) {
        std::string err = m_db ? sqlite3_errmsg(m_db) : "unknown error";
        throw std::runtime_error("Failed to open mission database '" + dbPath + "': " + err);
    }
    initSchema();
    std::cout << "[MISSION REPOSITORY] Connected to database: " << dbPath << "\n";
}

MissionRepository::~MissionRepository() {
    if (m_db) sqlite3_close(m_db);
}

void MissionRepository::execOrThrow(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Schema init failed: " + err);
    }
}

void MissionRepository::initSchema() {
    execOrThrow(
        "CREATE TABLE IF NOT EXISTS missions ("
        "  mission_id       TEXT PRIMARY KEY,"
        "  drone_id         TEXT NOT NULL,"
        "  pattern          TEXT NOT NULL,"
        "  cruise_altitude  REAL NOT NULL,"
        "  status           TEXT NOT NULL,"
        "  launch_time      INTEGER NOT NULL,"
        "  completion_time  INTEGER"
        ");"
    );
    execOrThrow(
        "CREATE TABLE IF NOT EXISTS mission_waypoints ("
        "  mission_id       TEXT NOT NULL,"
        "  waypoint_index   INTEGER NOT NULL,"
        "  latitude         REAL NOT NULL,"
        "  longitude        REAL NOT NULL,"
        "  altitude         REAL NOT NULL,"
        "  PRIMARY KEY (mission_id, waypoint_index),"
        "  FOREIGN KEY (mission_id) REFERENCES missions(mission_id)"
        ");"
    );
    execOrThrow("CREATE INDEX IF NOT EXISTS idx_missions_drone ON missions(drone_id);");
}

void MissionRepository::recordMissionLaunch(const std::string& missionId,
                                             const std::string& droneId,
                                             const std::string& pattern,
                                             const std::vector<GpsCoordinate>& waypoints,
                                             double cruiseAltitude,
                                             std::time_t launchTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    sqlite3_stmt* stmt = nullptr;
    const char* insertMission =
        "INSERT INTO missions (mission_id, drone_id, pattern, cruise_altitude, status, launch_time) "
        "VALUES (?, ?, ?, ?, 'ACTIVE', ?);";

    if (sqlite3_prepare_v2(m_db, insertMission, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[MISSION REPOSITORY ERROR] Failed to prepare mission insert: " << sqlite3_errmsg(m_db) << "\n";
        return;
    }
    sqlite3_bind_text(stmt, 1, missionId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, droneId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, cruiseAltitude);
    sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(launchTime));

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "[MISSION REPOSITORY ERROR] Failed to insert mission: " << sqlite3_errmsg(m_db) << "\n";
    }
    sqlite3_finalize(stmt);

    const char* insertWaypoint =
        "INSERT INTO mission_waypoints (mission_id, waypoint_index, latitude, longitude, altitude) "
        "VALUES (?, ?, ?, ?, ?);";

    for (std::size_t i = 0; i < waypoints.size(); ++i) {
        sqlite3_stmt* wpStmt = nullptr;
        if (sqlite3_prepare_v2(m_db, insertWaypoint, -1, &wpStmt, nullptr) != SQLITE_OK) {
            std::cerr << "[MISSION REPOSITORY ERROR] Failed to prepare waypoint insert: " << sqlite3_errmsg(m_db) << "\n";
            continue;
        }
        sqlite3_bind_text(wpStmt, 1, missionId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(wpStmt, 2, static_cast<int>(i));
        sqlite3_bind_double(wpStmt, 3, waypoints[i].latitude);
        sqlite3_bind_double(wpStmt, 4, waypoints[i].longitude);
        sqlite3_bind_double(wpStmt, 5, waypoints[i].altitude);

        if (sqlite3_step(wpStmt) != SQLITE_DONE) {
            std::cerr << "[MISSION REPOSITORY ERROR] Failed to insert waypoint " << i << ": " << sqlite3_errmsg(m_db) << "\n";
        }
        sqlite3_finalize(wpStmt);
    }

    std::cout << "[MISSION REPOSITORY] Recorded mission '" << missionId << "' ("
              << waypoints.size() << " waypoint(s)).\n";
}

void MissionRepository::recordMissionCompletion(const std::string& missionId,
                                                  const std::string& outcome,
                                                  std::time_t completionTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    sqlite3_stmt* stmt = nullptr;
    const char* update =
        "UPDATE missions SET status = ?, completion_time = ? WHERE mission_id = ?;";

    if (sqlite3_prepare_v2(m_db, update, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[MISSION REPOSITORY ERROR] Failed to prepare completion update: " << sqlite3_errmsg(m_db) << "\n";
        return;
    }
    sqlite3_bind_text(stmt, 1, outcome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(completionTime));
    sqlite3_bind_text(stmt, 3, missionId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "[MISSION REPOSITORY ERROR] Failed to update mission completion: " << sqlite3_errmsg(m_db) << "\n";
    }
    sqlite3_finalize(stmt);

    std::cout << "[MISSION REPOSITORY] Mission '" << missionId << "' marked " << outcome << ".\n";
}
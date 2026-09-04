#pragma once
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include "Services/MissionPlanner.h"
#include "Services/TelemetryManager.h"
#include "Common/Types.h"

class DroneSimulator {
public:
    DroneSimulator(MissionPlanner& planner,
                   TelemetryManager& telemetryManager,
                   std::shared_ptr<Drone> drone,
                   double speedMetersPerSec = 10.0)
        : m_planner(planner)
        , m_telemetryManager(telemetryManager)
        , m_drone(drone)
        , m_speed(speedMetersPerSec)
        , m_running(false) {}

    ~DroneSimulator() { stop(); }

    void start() {
        if (m_running) return;
        m_running = true;
        m_simThread = std::thread(&DroneSimulator::simulationLoop, this);
        std::cout << "[SIMULATOR] Started dynamic movement thread for drone: " << m_drone->getId() << "\n";
    }

    void stop() {
        if (!m_running) return;
        m_running = false;
        if (m_simThread.joinable()) m_simThread.join();
        std::cout << "[SIMULATOR] Stopped simulation thread for drone: " << m_drone->getId() << "\n";
    }

private:
    MissionPlanner& m_planner;
    TelemetryManager& m_telemetryManager;
    std::shared_ptr<Drone> m_drone;
    double m_speed;
    std::atomic<bool> m_running;
    std::thread m_simThread;

    double calculateDistance(const GpsCoordinate& a, const GpsCoordinate& b) const {
        double latDiff = (a.latitude - b.latitude) * 111000.0;
        double lonDiff = (a.longitude - b.longitude) * 111000.0 * std::cos(a.latitude * M_PI / 180.0);
        return std::sqrt(latDiff * latDiff + lonDiff * lonDiff);
    }

    double calculateHeading(const GpsCoordinate& from, const GpsCoordinate& to) const {
        double dLat = (to.latitude - from.latitude) * 111000.0;
        double dLon = (to.longitude - from.longitude) * 111000.0 * std::cos(from.latitude * M_PI / 180.0);
        double heading = std::atan2(dLon, dLat) * 180.0 / M_PI;
        if (heading < 0) heading += 360.0;
        return heading;
    }

    GpsCoordinate moveTowards(const GpsCoordinate& current, const GpsCoordinate& target, double stepMeters) const {
        double totalDist = calculateDistance(current, target);
        if (totalDist <= stepMeters || totalDist < 1e-6) return target;

        double ratio = stepMeters / totalDist;
        GpsCoordinate next;
        next.latitude = current.latitude + (target.latitude - current.latitude) * ratio;
        next.longitude = current.longitude + (target.longitude - current.longitude) * ratio;
        // FIXED: interpolate altitude too — previously snapped straight to
        // the target's altitude on the very first tick.
        next.altitude = current.altitude + (target.altitude - current.altitude) * ratio;
        return next;
    }

    void simulationLoop() {
        const int tickRateMs = 500;
        double stepSizeMeters = m_speed * (tickRateMs / 1000.0);

        while (m_running) {
            // FIXED: thread-safe copy instead of a raw pointer into
            // MissionPlanner's internal vector — never dangles even if a
            // concurrent completeMission()/recall erases the entry.
            ActiveMission missionSnapshot;
            if (!m_planner.getActiveMissionSnapshot(m_drone->getId(), missionSnapshot)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            GpsCoordinate targetPoint = missionSnapshot.currentTarget();
            GpsCoordinate droneCurrentPos = m_drone->getCurrentLocation();

            GpsCoordinate nextPos = moveTowards(droneCurrentPos, targetPoint, stepSizeMeters);
            double heading = calculateHeading(droneCurrentPos, nextPos);
            double instSpeed = calculateDistance(droneCurrentPos, nextPos) / (tickRateMs / 1000.0);

            m_drone->setCurrentLocation(nextPos);

            // FIXED: push the update into TelemetryManager — this is what
            // broadcastTelemetry() actually serializes to the frontend.
            // Without this, the map/telemetry cards never move.
            DroneTelemetry telem;
            telem.droneId = m_drone->getId();
            telem.latitude = nextPos.latitude;
            telem.longitude = nextPos.longitude;
            telem.altitude = nextPos.altitude;
            telem.batteryLevel = m_drone->getBatteryLevel();
            telem.speed = instSpeed;
            telem.heading = heading;
            telem.state = m_drone->getState();
            m_telemetryManager.updateDroneTelemetry(m_drone->getId(), telem);

            std::cout << "[SIM] Drone " << m_drone->getId()
                      << " -> (" << nextPos.latitude << ", " << nextPos.longitude
                      << ") heading " << heading << "°\n";

            if (calculateDistance(nextPos, targetPoint) < 1.0) {
                if (missionSnapshot.isFinalWaypoint()) {
                    std::cout << "[SIM] Drone " << m_drone->getId() << " reached FINAL waypoint. Awaiting Finish.\n";
                    m_planner.markFinalWaypointReached(m_drone->getId());
                } else {
                    std::cout << "[SIM] Drone " << m_drone->getId() << " reached waypoint. Advancing.\n";
                    m_planner.advanceMissionWaypoint(m_drone->getId());
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(tickRateMs));
        }
    }
};
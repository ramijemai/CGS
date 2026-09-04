#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include <mavsdk/mavsdk.hpp>
#include <mavsdk/plugins/action/action.hpp>
#include <mavsdk/plugins/telemetry/telemetry.hpp>
#include <mavsdk/plugins/mission/mission.hpp>

#include "Domain/Drone.h"
#include "Domain/Bunker.h"
#include "Services/MissionPlanner.h"
#include "Services/TelemetryManager.h"
#include "Services/RecoveryService.h"

// Drives ONE real (SITL-backed) drone over actual MAVLink via MAVSDK,
// replacing DroneSimulator's synthetic physics for that specific drone.
// Every other drone in the fleet continues using DroneSimulator untouched.
class MavlinkFlightController {
public:
    MavlinkFlightController(MissionPlanner& missionPlanner,
                             TelemetryManager& telemetryManager,
                             RecoveryService& recoveryService,
                             const Bunker& bunker,
                             std::shared_ptr<Drone> drone);
    ~MavlinkFlightController();

    // Connects to SITL and blocks until the vehicle reports healthy.
    // Example: "udpin://0.0.0.0:14540"
    // NOTE: mavsdk::Mavsdk's constructor signature differs between MAVSDK
    // v1.x (default-constructible) and v2.x (requires a Configuration).
    // If this doesn't compile as written, check your installed version's
    // getting-started docs for the correct constructor.
    bool connect(const std::string& connectionUrl);

    // Starts the background thread watching MissionPlanner for new
    // missions targeting this drone and dispatching them over real MAVLink.
    void start();
    void stop();

    // Commands a real RTL. Actual docking (bay allocation, telemetry
    // unregister, mission completion) happens ASYNCHRONOUSLY once the
    // vehicle reports it has actually landed — see finalizeLanding().
    // Returns whether the RTL command itself was accepted, NOT whether
    // the drone is docked yet.
    bool requestReturnToLaunch();

    bool ownsDrone(const std::string& droneId) const;
    bool isOnGround() const;

private:
    MissionPlanner& m_missionPlanner;
    TelemetryManager& m_telemetryManager;
    RecoveryService& m_recoveryService;
    const Bunker& m_bunker;
    std::shared_ptr<Drone> m_drone;
    bool m_missionAborted{false};
    mavsdk::Mavsdk m_mavsdk;
    std::shared_ptr<mavsdk::System> m_system;
    std::unique_ptr<mavsdk::Action> m_action;
    std::unique_ptr<mavsdk::Telemetry> m_telemetry;
    std::unique_ptr<mavsdk::Mission> m_mission;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_awaitingDock{false};
    std::atomic<double> m_lastGroundSpeed{0.0};
    std::atomic<double> m_lastHeading{0.0};
    std::thread m_pollThread;

    void subscribeTelemetry();
    void pollLoop();
    void dispatchRealMission(const ActiveMission& mission);
    void finalizeLanding();
};
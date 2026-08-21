#include "Services/MavlinkFlightController.h"
#include <chrono>
#include <cmath>
#include <future>
#include <iostream>

namespace {
    DroneState mapFlightMode(mavsdk::Telemetry::FlightMode mode) {
        using FlightMode = mavsdk::Telemetry::FlightMode;
        switch (mode) {
            case FlightMode::ReturnToLaunch:
                return DroneState::ReturningToBunker;
            case FlightMode::Land:
                return DroneState::Landing;
            case FlightMode::Stabilized:
            case FlightMode::Hold:
            case FlightMode::Manual:
            case FlightMode::Ready:
                return DroneState::Idle; // <-- Map ground modes to Idle!
            default:
                return DroneState::InFlight;
        }
    }
}

MavlinkFlightController::MavlinkFlightController(MissionPlanner& missionPlanner,
                                                 TelemetryManager& telemetryManager,
                                                 RecoveryService& recoveryService,
                                                 const Bunker& bunker,
                                                 std::shared_ptr<Drone> drone)
    : m_missionPlanner(missionPlanner)
    , m_telemetryManager(telemetryManager)
    , m_recoveryService(recoveryService)
    , m_bunker(bunker)
    , m_drone(std::move(drone))
    , m_mavsdk(mavsdk::Mavsdk::Configuration{mavsdk::ComponentType::GroundStation}) {}

MavlinkFlightController::~MavlinkFlightController() {
    stop();
}

bool MavlinkFlightController::connect(const std::string& connectionUrl) {
    auto connResult = m_mavsdk.add_any_connection(connectionUrl);
    if (connResult != mavsdk::ConnectionResult::Success) {
        std::cerr << "[MAVLINK] Connection failed for '" << connectionUrl << "'\n";
        return false;
    }

    std::cout << "[MAVLINK] Waiting for SITL system on " << connectionUrl << "...\n";
    auto prom = std::promise<std::shared_ptr<mavsdk::System>>{};
    auto fut = prom.get_future();
    
    m_mavsdk.subscribe_on_new_system([this, &prom]() {
        if (!m_mavsdk.systems().empty()) {
            auto system = m_mavsdk.systems().back();
            if (system && system->has_autopilot()) {
                prom.set_value(system);
            }
        }
    });

    if (fut.wait_for(std::chrono::seconds(15)) == std::future_status::timeout) {
        std::cerr << "[MAVLINK] Timed out waiting for SITL system.\n";
        return false;
    }

    m_system = fut.get();
    if (!m_system) return false;

    m_action = std::make_unique<mavsdk::Action>(m_system);
    m_telemetry = std::make_unique<mavsdk::Telemetry>(m_system);
    m_mission = std::make_unique<mavsdk::Mission>(m_system);

    std::cout << "[MAVLINK] Waiting for vehicle health checks...\n";
    while (m_telemetry && !m_telemetry->health_all_ok()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "[MAVLINK] Vehicle ready for '" << m_drone->getId() << "'.\n";
    m_drone->setState(DroneState::Idle);

    subscribeTelemetry();
    return true;
}

void MavlinkFlightController::subscribeTelemetry() {
    if (!m_telemetry) return;

    m_telemetry->subscribe_position([this](mavsdk::Telemetry::Position position) {
        if (!m_drone) return;
        GpsCoordinate loc{position.latitude_deg, position.longitude_deg, position.relative_altitude_m};
        m_drone->setCurrentLocation(loc);

        DroneTelemetry telem;
        telem.droneId = m_drone->getId();
        telem.latitude = loc.latitude;
        telem.longitude = loc.longitude;
        telem.altitude = loc.altitude;
        telem.batteryLevel = m_drone->getBatteryLevel();
        telem.speed = m_lastGroundSpeed.load();
        telem.heading = m_lastHeading.load();
        telem.state = m_drone->getState();
        m_telemetryManager.updateDroneTelemetry(m_drone->getId(), telem);
    });

    m_telemetry->subscribe_battery([this](mavsdk::Telemetry::Battery battery) {
        if (m_drone) m_drone->setBatteryLevel(battery.remaining_percent * 100.0);
    });

    m_telemetry->subscribe_flight_mode([this](mavsdk::Telemetry::FlightMode mode) {
        if (m_drone) m_drone->setState(mapFlightMode(mode));
    });

    m_telemetry->subscribe_velocity_ned([this](mavsdk::Telemetry::VelocityNed v) {
        double speed = std::sqrt(v.north_m_s * v.north_m_s + v.east_m_s * v.east_m_s);
        m_lastGroundSpeed.store(speed);
        double heading = std::atan2(v.east_m_s, v.north_m_s) * 180.0 / M_PI;
        if (heading < 0) heading += 360.0;
        m_lastHeading.store(heading);
    });

    m_telemetry->subscribe_landed_state([this](mavsdk::Telemetry::LandedState state) {
        if (state == mavsdk::Telemetry::LandedState::OnGround && m_awaitingDock.exchange(false)) {
            finalizeLanding();
        }
    });
}

void MavlinkFlightController::dispatchRealMission(const ActiveMission& mission) {
    if (!m_mission || !m_action || mission.waypoints.empty()) return;

    mavsdk::Mission::MissionPlan plan;
    for (const auto& wp : mission.waypoints) {
        mavsdk::Mission::MissionItem item{};
        item.latitude_deg = wp.latitude;
        item.longitude_deg = wp.longitude;
        item.relative_altitude_m = static_cast<float>(mission.cruiseAltitude);
        item.speed_m_s = 5.0f;
        item.is_fly_through = true;
        plan.mission_items.push_back(item);
    }

    m_mission->set_return_to_launch_after_mission(false);

    std::cout << "[MAVLINK] Uploading mission '" << mission.missionId << "'...\n";
    if (m_mission->upload_mission(plan) != mavsdk::Mission::Result::Success) {
        std::cerr << "[MAVLINK] Mission upload failed.\n";
        return;
    }

    // 1. Set Takeoff Altitude
    m_action->set_takeoff_altitude(static_cast<float>(mission.cruiseAltitude));

    // 2. Arm
    if (m_action->arm() != mavsdk::Action::Result::Success) {
        std::cerr << "[MAVLINK] Arming failed.\n";
        return;
    }

    // 3. Perform Guided Takeoff
    if (m_action->takeoff() != mavsdk::Action::Result::Success) {
        std::cerr << "[MAVLINK] Takeoff failed.\n";
        return;
    }

    // 4. Wait until the drone reaches initial altitude before switching to AUTO mode
    std::cout << "[MAVLINK] Taking off... Waiting for altitude clearance.\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 5. Start Mission
    if (m_mission->start_mission() != mavsdk::Mission::Result::Success) {
        std::cerr << "[MAVLINK] Mission start failed.\n";
        return;
    }

    std::cout << "[MAVLINK] Mission '" << mission.missionId << "' airborne on real MAVLink.\n";
}

void MavlinkFlightController::start() {
    if (m_running) return;
    m_running = true;
    m_pollThread = std::thread(&MavlinkFlightController::pollLoop, this);
}

void MavlinkFlightController::stop() {
    if (!m_running) return;
    m_running = false;
    if (m_pollThread.joinable()) m_pollThread.join();
}

void MavlinkFlightController::pollLoop() {
    std::string lastSeenMissionId;
    while (m_running) {
        ActiveMission mission;
        if (m_missionPlanner.getActiveMissionSnapshot(m_drone->getId(), mission)) {
            if (mission.missionId != lastSeenMissionId) {
                lastSeenMissionId = mission.missionId;
                dispatchRealMission(mission);
            }
        } else {
            lastSeenMissionId.clear();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

bool MavlinkFlightController::requestReturnToLaunch() {
    if (!m_action) return false;
    if (m_action->return_to_launch() != mavsdk::Action::Result::Success) return false;
    m_drone->setState(DroneState::ReturningToBunker);
    m_awaitingDock = true;
    return true;
}

void MavlinkFlightController::finalizeLanding() {
    if (m_recoveryService.executeRecoveryAndDocking(m_drone, m_bunker)) {
        m_telemetryManager.unregisterActiveDrone(m_drone->getId());
        m_missionPlanner.completeMission(m_drone->getId(), "COMPLETED");
    }
}

bool MavlinkFlightController::ownsDrone(const std::string& droneId) const {
    return m_drone && m_drone->getId() == droneId;
}
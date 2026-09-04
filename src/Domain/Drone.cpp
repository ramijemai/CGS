#include "Domain/Drone.h"
#include <algorithm>
#include <utility>

Drone::Drone(std::string id, double batteryLevel)
    : m_id(std::move(id)), m_batteryLevel(std::clamp(batteryLevel, 0.0, 100.0)), m_state(DroneState::Idle) {}

const std::string& Drone::getId() const {
    return m_id;   
}

double Drone::getBatteryLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_batteryLevel;
}

void Drone::setBatteryLevel(double level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_batteryLevel = std::clamp(level, 0.0, 100.0);
}

GpsCoordinate Drone::getCurrentLocation() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentLocation;
}

void Drone::setCurrentLocation(const GpsCoordinate& location) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentLocation = location;
}

DroneState Drone::getState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

void Drone::setState(DroneState state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = state;
}

bool Drone::isReadyForMission() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const bool batteryReady = m_batteryLevel >= 20.0;
    return (m_state == DroneState::Ready || m_state == DroneState::Idle || m_state == DroneState::Docked)
        && batteryReady;
}
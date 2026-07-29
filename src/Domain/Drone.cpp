#include "Domain/Drone.h"
#include <algorithm>
#include <utility>

Drone::Drone(std::string id, double batteryLevel)
    : m_id(std::move(id)), m_batteryLevel(std::clamp(batteryLevel, 0.0, 100.0)), m_state(DroneState::Idle) {}

const std::string& Drone::getId() const {
    return m_id;
}

double Drone::getBatteryLevel() const {
    return m_batteryLevel;
}

void Drone::setBatteryLevel(double level) {
    m_batteryLevel = std::clamp(level, 0.0, 100.0);
}

const GpsCoordinate& Drone::getCurrentLocation() const {
    return m_currentLocation;
}


void Drone::setCurrentLocation(const GpsCoordinate& location) {
    m_currentLocation = location;
}

DroneState Drone::getState() const {
    return m_state;
}

void Drone::setState(DroneState state) {
    m_state = state;
}

bool Drone::isReadyForMission() const {
    return (m_state == DroneState::Ready || m_state == DroneState::Idle || m_state == DroneState::Charging)
        && m_batteryLevel >= 90.0;
}
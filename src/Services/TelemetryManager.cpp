#include "Services/TelemetryManager.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

void DroneTelemetry::print() const {
    std::cout << "[TELEMETRY] Drone: " << droneId 
              << " | Position: (" << std::fixed << std::setprecision(4) << latitude << ", " << longitude << ", " << altitude << "m)"
              << " | Battery: " << batteryLevel << "%"
              << " | Speed: " << speed << " m/s"
              << " | Heading: " << heading << "°\n";
}

void TelemetryMessage::print() const {
    std::cout << "[COMM] To: " << droneId << " | Command: " << static_cast<int>(command)
              << " | Target: (" << targetLatitude << ", " << targetLongitude << ", " << targetAltitude << "m)"
              << " | Message: " << messageText << "\n";
}

void TelemetryManager::registerActiveDrone(const std::shared_ptr<Drone>& drone, const GpsCoordinate& initialPosition) {
    if (!drone) return;
    DroneTelemetry telem;
    telem.droneId = drone->getId();
    telem.latitude = initialPosition.latitude;
    telem.longitude = initialPosition.longitude;
    telem.altitude = initialPosition.altitude;
    telem.batteryLevel = drone->getBatteryLevel();
    telem.state = drone->getState();
    telem.lastUpdate = std::time(nullptr);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeDrones[drone->getId()] = telem;
    m_droneRegistry[drone->getId()] = drone;
    std::cout << "[TELEMETRY MANAGER] Drone '" << drone->getId() << "' registered as ACTIVE.\n";
}

void TelemetryManager::unregisterActiveDrone(const std::string& droneId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeDrones.find(droneId);
    if (it != m_activeDrones.end()) {
        m_telemetryHistory.push_back(it->second);
        m_activeDrones.erase(it);
        m_droneRegistry.erase(droneId);
        std::cout << "[TELEMETRY MANAGER] Drone '" << droneId << "' unregistered (LANDED).\n";
    }
}

std::shared_ptr<Drone> TelemetryManager::getActiveDronePtr(const std::string& droneId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_droneRegistry.find(droneId);
    return it != m_droneRegistry.end() ? it->second : nullptr;
}

void TelemetryManager::updateDroneTelemetry(const std::string& droneId, const DroneTelemetry& telemetry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeDrones.find(droneId);
    if (it != m_activeDrones.end()) {
        it->second = telemetry;
        it->second.lastUpdate = std::time(nullptr);
        m_telemetryHistory.push_back(telemetry);
        trimHistory();
    }
}

void TelemetryManager::updateDroneBattery(const std::string& droneId, double batteryLevel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeDrones.find(droneId);
    if (it != m_activeDrones.end()) {
        it->second.batteryLevel = batteryLevel;
        it->second.lastUpdate = std::time(nullptr);
    }
}

std::vector<DroneTelemetry> TelemetryManager::getAllActiveDronesTelemetry() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DroneTelemetry> result;
    for (const auto& pair : m_activeDrones) result.push_back(pair.second);
    return result;
}

void TelemetryManager::sendCommand(const std::string& droneId, const TelemetryMessage& message) {
    std::cout << "[TELEMETRY] Sending command to drone: " << droneId << "\n";
    message.print();
    TelemetryMessage logMsg = message;
    logMsg.timestamp = std::time(nullptr);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_commandHistory.push_back(logMsg);
    trimHistory();
}

void TelemetryManager::printActiveDronesTelemetry() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << "\n========== ACTIVE DRONES TELEMETRY ==========\n";
    std::cout << "Total active drones: " << m_activeDrones.size() << "\n\n";
    for (const auto& pair : m_activeDrones) pair.second.print();
    std::cout << "============================================\n";
}

int TelemetryManager::getActiveDroneCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_activeDrones.size());
}

std::vector<TelemetryMessage> TelemetryManager::getCommandHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_commandHistory;
}

void TelemetryManager::trimHistory() {
    // Caller must already hold m_mutex.
    if (m_telemetryHistory.size() > kMaxHistory) {
        m_telemetryHistory.erase(m_telemetryHistory.begin(), m_telemetryHistory.begin() + (m_telemetryHistory.size() - kMaxHistory));
    }
    if (m_commandHistory.size() > kMaxHistory) {
        m_commandHistory.erase(m_commandHistory.begin(), m_commandHistory.begin() + (m_commandHistory.size() - kMaxHistory));
    }
}
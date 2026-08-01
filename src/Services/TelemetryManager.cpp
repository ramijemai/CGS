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

    m_activeDrones[drone->getId()] = telem;
    m_droneRegistry[drone->getId()] = drone;   // <-- retain the real pointer
    std::cout << "[TELEMETRY MANAGER] Drone '" << drone->getId() << "' registered as ACTIVE.\n";
}

void TelemetryManager::unregisterActiveDrone(const std::string& droneId) {
    auto it = m_activeDrones.find(droneId);
    if (it != m_activeDrones.end()) {
        m_telemetryHistory.push_back(it->second);
        m_activeDrones.erase(it);
        m_droneRegistry.erase(droneId);   // <-- drop it once the drone is docked again
        std::cout << "[TELEMETRY MANAGER] Drone '" << droneId << "' unregistered (LANDED).\n";
    }
}

std::shared_ptr<Drone> TelemetryManager::getActiveDronePtr(const std::string& droneId) const {
    auto it = m_droneRegistry.find(droneId);
    return it != m_droneRegistry.end() ? it->second : nullptr;
}

void TelemetryManager::updateDroneTelemetry(const std::string& droneId, const DroneTelemetry& telemetry) {
    auto it = m_activeDrones.find(droneId);
    if (it != m_activeDrones.end()) {
        it->second = telemetry;
        it->second.lastUpdate = std::time(nullptr);
        m_telemetryHistory.push_back(telemetry);
        trimHistory();
    }
}

DroneTelemetry* TelemetryManager::getDroneTelemetry(const std::string& droneId) {
    auto it = m_activeDrones.find(droneId);
    if (it != m_activeDrones.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<DroneTelemetry> TelemetryManager::getAllActiveDronesTelemetry() const {
    std::vector<DroneTelemetry> result;
    for (const auto& pair : m_activeDrones) {
        result.push_back(pair.second);
    }
    return result;
}

void TelemetryManager::sendCommand(const std::string& droneId, const TelemetryMessage& message) {
    std::cout << "[TELEMETRY] Sending command to drone: " << droneId << "\n";
    message.print();
    
    TelemetryMessage logMsg = message;
    logMsg.timestamp = std::time(nullptr);
    m_commandHistory.push_back(logMsg);
    trimHistory();
}

void TelemetryManager::receiveStatusUpdate(const std::string& droneId, const DroneTelemetry& telemetry) {
    std::cout << "[TELEMETRY] Status update from: " << droneId << "\n";
    updateDroneTelemetry(droneId, telemetry);
}

void TelemetryManager::printActiveDronesTelemetry() const {
    std::cout << "\n========== ACTIVE DRONES TELEMETRY ==========\n";
    std::cout << "Total active drones: " << m_activeDrones.size() << "\n\n";
    
    for (const auto& pair : m_activeDrones) {
        pair.second.print();
    }
    
    std::cout << "============================================\n";
}

int TelemetryManager::getActiveDroneCount() const {
    return m_activeDrones.size();
}

std::vector<TelemetryMessage> TelemetryManager::getCommandHistory() const {
    return m_commandHistory;
}

void TelemetryManager::trimHistory() {
    if (m_telemetryHistory.size() > kMaxHistory) {
        m_telemetryHistory.erase(
            m_telemetryHistory.begin(),
            m_telemetryHistory.begin() + (m_telemetryHistory.size() - kMaxHistory)
        );
    }
    if (m_commandHistory.size() > kMaxHistory) {
        m_commandHistory.erase(
            m_commandHistory.begin(),
            m_commandHistory.begin() + (m_commandHistory.size() - kMaxHistory)
        );
    }
}
/*
void TelemetryManager::simulateDroneFlight(const std::string& droneId, const GpsCoordinate& target, double flightTime) {
    auto telem = getDroneTelemetry(droneId);
    if (!telem) return;

    // Simulate movement towards target
    double latDelta = target.latitude - telem->latitude;
    double lonDelta = target.longitude - telem->longitude;
    double altDelta = target.altitude - telem->altitude;
    
    // Calculate distance (simplified)
    double distance = std::sqrt(latDelta * latDelta + lonDelta * lonDelta);
    double speed = (distance / flightTime) * 1000; // Convert to m/s
    double heading = std::atan2(lonDelta, latDelta) * 180.0 / M_PI;
    if (heading < 0) heading += 360.0;

    // Update telemetry
    telem->latitude = target.latitude;
    telem->longitude = target.longitude;
    telem->altitude = target.altitude;
    telem->speed = speed;
    telem->heading = heading;
    
    // Simulate battery drain (0.5% per minute of flight)
    telem->batteryLevel -= (flightTime / 60.0) * 0.5;
    telem->batteryLevel = std::max(0.0, telem->batteryLevel);
    
    telem->lastUpdate = std::time(nullptr);
    
    std::cout << "[FLIGHT SIMULATION] Drone '" << droneId << "' reached target. "
              << "Battery: " << telem->batteryLevel << "%\n";
}
*/

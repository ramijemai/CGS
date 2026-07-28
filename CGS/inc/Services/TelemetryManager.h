#pragma once
#include "Domain/Drone.h"
#include "Common/Types.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <ctime>

// Real-time telemetry data for a drone in flight
struct DroneTelemetry {
    std::string droneId;
    double latitude{0.0};
    double longitude{0.0};
    double altitude{0.0};
    double batteryLevel{0.0};
    double speed{0.0};  // m/s
    double heading{0.0}; // degrees (0-360)
    DroneState state;
    std::time_t lastUpdate{0};
    
    void print() const;
};

// Command to send to a drone
enum class DroneCommand {
    UPDATE_TARGET,
    RETURN_TO_BUNKER,
    HOLD_POSITION,
    EMERGENCY_LAND,
    STATUS_CHECK
};

// Communication message between bunker and drone
struct TelemetryMessage {
    std::string droneId;
    DroneCommand command;
    double targetLatitude{0.0};
    double targetLongitude{0.0};
    double targetAltitude{0.0};
    std::string messageText;
    std::time_t timestamp{0};
    
    void print() const;
};

class TelemetryManager {
public:
    TelemetryManager() = default;

    // Register a drone as active (in flight)
    void registerActiveDrone(const std::shared_ptr<Drone>& drone, const GpsCoordinate& initialPosition);

    // Unregister a drone (landed/recovered)
    void unregisterActiveDrone(const std::string& droneId);

    // Update telemetry data from drone
    void updateDroneTelemetry(const std::string& droneId, const DroneTelemetry& telemetry);

    // Get current telemetry for a drone
    DroneTelemetry* getDroneTelemetry(const std::string& droneId);

    // Get all active drones telemetry
    std::vector<DroneTelemetry> getAllActiveDronesTelemetry() const;

    // Send command to drone
    void sendCommand(const std::string& droneId, const TelemetryMessage& message);

    // Receive status update from drone
    void receiveStatusUpdate(const std::string& droneId, const DroneTelemetry& telemetry);

    // Print all active drones telemetry
    void printActiveDronesTelemetry() const;

    // Get count of active drones
    int getActiveDroneCount() const;

    // Simulate drone flight (update positions over time)
    void simulateDroneFlight(const std::string& droneId, const GpsCoordinate& target, double flightTime);

private:
    std::map<std::string, DroneTelemetry> m_activeDrones;
    std::vector<TelemetryMessage> m_commandHistory;
    std::vector<DroneTelemetry> m_telemetryHistory;
};

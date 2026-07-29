#pragma once
#include "Common/Types.h"
#include <string>

class Drone {
public:
    explicit Drone(std::string id, double batteryLevel = 100.0);

    const std::string& getId() const;
    double getBatteryLevel() const;
    void setBatteryLevel(double level);
    const GpsCoordinate& getCurrentLocation() const;
    void setCurrentLocation(const GpsCoordinate& location);
    DroneState getState() const;
    void setState(DroneState state);

    bool isReadyForMission() const;

private:
    std::string m_id;
    double m_batteryLevel;
    DroneState m_state;
    GpsCoordinate m_currentLocation;
};
#pragma once
#include <string>

struct GpsCoordinate {
    double latitude{0.0};
    double longitude{0.0};
    double altitude{0.0};
};

enum class DroneState {
    Idle,
    Charging,
    Ready,
    InFlight,
    ReturningToBunker, // Phase 2
    Landing,           // Phase 2
    Fault
};

enum class BayState {
    Vacant,
    Occupied,
    Maintenance
};

enum class HatchState {
    Closed,
    Opening,
    Open,
    Closing
};
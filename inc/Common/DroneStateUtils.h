#pragma once
#include "Domain/Drone.h"
#include <string>

namespace DroneStateUtils {

inline std::string toString(DroneState state) {
    switch (state) {
        case DroneState::Idle:              return "IDLE";
        case DroneState::InFlight:          return "IN_FLIGHT";
        case DroneState::ReturningToBunker: return "RETURNING";
        case DroneState::Landing:           return "LANDING";
        case DroneState::Docked:            return "DOCKED";
        case DroneState::Charging:          return "CHARGING";
        case DroneState::Fault:             return "FAULT";
        default:                            return "UNKNOWN";
    }
}

} // namespace DroneStateUtils
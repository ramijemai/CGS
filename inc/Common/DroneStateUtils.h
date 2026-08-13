#pragma once
#include "Domain/Drone.h"
#include <string>

namespace DroneStateUtils {

inline std::string toString(DroneState state) {
    switch (state) {
        case DroneState::InFlight:          return "IN_FLIGHT";
        case DroneState::ReturningToBunker: return "RETURNING";
        case DroneState::Landing:           return "LANDING";
        case DroneState::Fault:             return "FAULT";
        default:                             return "DOCKED";
    }
}

} // namespace DroneStateUtils
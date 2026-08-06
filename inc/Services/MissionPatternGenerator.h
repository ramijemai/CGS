#pragma once
#include "Common/Types.h"
#include "Common/GeoUtils.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Pure waypoint-generation — no drone/service dependencies, easy to unit
// test in isolation from CapacityEngine/TelemetryManager.
namespace MissionPatternGenerator {

// Generates `numPoints` waypoints evenly spaced around a circle of
// `radiusMeters` centered on `center`, at fixed `altitude`. Minimum 3
// points enforced — an "orbit" with fewer isn't meaningful.
inline std::vector<GpsCoordinate> generateOrbit(const GpsCoordinate& center,
                                                 double radiusMeters,
                                                 double altitude,
                                                 int numPoints = 8) {
    std::vector<GpsCoordinate> waypoints;
    numPoints = std::max(numPoints, 3);
    radiusMeters = std::max(radiusMeters, 1.0);

    for (int i = 0; i < numPoints; ++i) {
        double angleRad = (2.0 * M_PI * i) / numPoints;
        double dNorth = radiusMeters * std::cos(angleRad);
        double dEast  = radiusMeters * std::sin(angleRad);
        GpsCoordinate wp = GeoUtils::offsetMeters(center, dNorth, dEast);
        wp.altitude = altitude;
        waypoints.push_back(wp);
    }
    return waypoints;
}

} // namespace MissionPatternGenerator
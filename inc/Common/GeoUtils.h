#pragma once
#include "Common/Types.h"
#include <cmath>

// Local flat-earth approximation. Valid for small footprints (up to a few
// km) — this is the scale of a roof, construction site, or industrial
// campus, NOT the scale of a long utility/pipeline corridor. Do not reuse
// this for multi-kilometer distances; see the note on
// TelemetryManager::simulateDroneFlight's degree-based distance bug for why.
namespace GeoUtils {

constexpr double kMetersPerDegreeLat = 111320.0;

inline double metersPerDegreeLon(double atLatitudeDeg) {
    return kMetersPerDegreeLat * std::cos(atLatitudeDeg * M_PI / 180.0);
}

// Returns a coordinate offset from `base` by dNorthMeters/dEastMeters.
// Altitude is copied from `base` unless overwritten by the caller afterward.
inline GpsCoordinate offsetMeters(const GpsCoordinate& base, double dNorthMeters, double dEastMeters) {
    GpsCoordinate result = base;
    result.latitude  += dNorthMeters / kMetersPerDegreeLat;
    result.longitude += dEastMeters  / metersPerDegreeLon(base.latitude);
    return result;
}

} // namespace GeoUtils
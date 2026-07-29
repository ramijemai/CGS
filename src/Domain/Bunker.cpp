#include "Domain/Bunker.h"

Bunker::Bunker(const GpsCoordinate& location)
    : m_gpsLocation(location) {}

void Bunker::setGpsLocation(const GpsCoordinate& location) {
    m_gpsLocation = location;
}

const GpsCoordinate& Bunker::getGpsLocation() const {
    return m_gpsLocation;
}

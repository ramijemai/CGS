#pragma once
#include "Common/Types.h"

class Bunker {
public:
    Bunker() = default;
    explicit Bunker(const GpsCoordinate& location);

    void setGpsLocation(const GpsCoordinate& location);
    const GpsCoordinate& getGpsLocation() const;

private:
    GpsCoordinate m_gpsLocation{};
};

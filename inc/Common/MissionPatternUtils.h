#pragma once
#include "Common/MissionPattern.h"
#include <string>

namespace MissionPatternUtils {

inline std::string toString(MissionPattern pattern) {
    switch (pattern) {
        case MissionPattern::WAYPOINT_PATH: return "WAYPOINT_PATH";
        case MissionPattern::SINGLE_POINT:
        default:                             return "SINGLE_POINT";
    }
}

} // namespace MissionPatternUtils
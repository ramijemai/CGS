#pragma once

enum class MissionPattern {
    SINGLE_POINT,    // one target, existing behavior
    WAYPOINT_PATH    // ordered sequence of manually-placed waypoints
};
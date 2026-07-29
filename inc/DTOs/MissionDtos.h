// DTOs/MissionDtos.h
#pragma once
#include <string>

struct LaunchMissionRequestDto {
    double targetLat;
    double targetLon;
    double targetAlt;
    double durationSeconds;
};

struct ApiResponseDto {
    bool success;
    std::string message;
};
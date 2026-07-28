cmake_minimum_required(VERSION 3.16)
project(BunkerProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

set(SOURCES
    src/Domain/Drone.cpp
    src/Domain/BaySlot.cpp
    src/Services/CapacityEngine.cpp
    src/Services/MissionPlanner.cpp
    src/Services/RecoveryService.cpp
    src/Services/TelemetryManager.cpp
    src/main.cpp
)

add_executable(BunkerProject ${SOURCES})

target_compile_options(BunkerProject PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
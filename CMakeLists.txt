cmake_minimum_required(VERSION 3.16)
project(BunkerProject VERSION 1.0.0 LANGUAGES CXX)

# Silence CMake 3.30+ FindBoost deprecation warning
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Required for std::thread and Boost.Asio networking
find_package(Threads REQUIRED)

# Boost.System is header-only in Boost 1.89+, so we only need 'json' as a component
find_package(Boost REQUIRED COMPONENTS json)
find_package(SQLite3 REQUIRED)

# Find MAVSDK package
find_package(MAVSDK REQUIRED)

set(SOURCES
    src/Domain/Drone.cpp
    src/Domain/BaySlot.cpp
    src/Domain/Bunker.cpp
    src/Services/CapacityEngine.cpp
    src/Services/MissionPlanner.cpp
    src/Services/RecoveryService.cpp
    src/Services/MissionRepository.cpp
    src/Services/TelemetryManager.cpp
    src/Controller/MissionController.cpp
    src/Controller/TelemetryWebSocketController.cpp
    src/Services/MavlinkFlightController.cpp
    src/main.cpp
)

add_executable(BunkerProject ${SOURCES})

target_include_directories(BunkerProject PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_options(BunkerProject PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Link header-only Boost targets, Boost.JSON, SQLite, and system threads
target_link_libraries(BunkerProject PRIVATE
    MAVSDK::mavsdk
    Boost::headers
    Boost::json
    SQLite::SQLite3
    Threads::Threads
)
# CGS (Command & Ground Station) — Current Project Summary

## What this project is doing
This repository is a C++17 prototype for a drone bunker and mission-control simulation. It models a small ground-station workflow where drones can be housed in bay slots, charged, launched on inspection missions, and later recovered. The current implementation is focused on core orchestration and basic API/WebSocket handling rather than a full production system.

## Current architecture
The project is organized into three main layers:

- Domain layer: drone, bay slot, and bunker entities
- Service layer: mission planning, capacity management, recovery, and telemetry handling
- Controller layer: REST endpoints and WebSocket command handling

The server entry point is in [src/main.cpp](src/main.cpp). It starts a Boost.Beast HTTP/WebSocket server on port 18080 and wires together the core services and controllers.

## Main modules

### Domain
- [inc/Domain/Drone.h](inc/Domain/Drone.h) and [src/Domain/Drone.cpp](src/Domain/Drone.cpp): represent a drone with an ID, battery level, state, and location.
- [inc/Domain/BaySlot.h](inc/Domain/BaySlot.h) and [src/Domain/BaySlot.cpp](src/Domain/BaySlot.cpp): represent a bay that can hold a drone, manage hatch state, and simulate charging.
- [inc/Domain/Bunker.h](inc/Domain/Bunker.h) and [src/Domain/Bunker.cpp](src/Domain/Bunker.cpp): store the bunker location.

### Services
- [inc/Services/CapacityEngine.h](inc/Services/CapacityEngine.h) and [src/Services/CapacityEngine.cpp](src/Services/CapacityEngine.cpp): manage the inventory of bay slots.
- [inc/Services/MissionPlanner.h](inc/Services/MissionPlanner.h) and [src/Services/MissionPlanner.cpp](src/Services/MissionPlanner.cpp): find a ready drone and dispatch it for a mission.
- [inc/Services/RecoveryService.h](inc/Services/RecoveryService.h) and [src/Services/RecoveryService.cpp](src/Services/RecoveryService.cpp): handle return-to-bunker recovery flow.
- [inc/Services/TelemetryManager.h](inc/Services/TelemetryManager.h) and [src/Services/TelemetryManager.cpp](src/Services/TelemetryManager.cpp): track active drones and command history.

### Controllers
- [inc/Controller/MissionController.h](inc/Controller/MissionController.h) and [src/Controller/MissionController.cpp](src/Controller/MissionController.cpp): expose REST endpoints for launching missions and retrieving bunker status.
- [inc/Controller/TelemetryWebSocketController.h](inc/Controller/TelemetryWebSocketController.h) and [src/Controller/TelemetryWebSocketController.cpp](src/Controller/TelemetryWebSocketController.cpp): handle WebSocket command messages.

## What the runtime flow looks like
1. The server starts from [src/main.cpp](src/main.cpp).
2. A client can send a mission launch request to POST /api/v1/missions/launch.
3. The mission controller passes the request to the mission planner.
4. The planner selects a ready drone, marks it as in flight, and removes it from its bay.
5. A client can query the bunker state with GET /api/v1/bunker/status.
6. WebSocket clients can send commands to /ws/telemetry and receive an acknowledgement message.

## Current state of the project
At this stage, the project already has the main building blocks for a simulated drone operations system:

- bay and drone state management
- mission dispatch flow
- basic recovery workflow
- REST and WebSocket interfaces
- in-memory telemetry tracking

It is still best understood as a prototype or simulation. Some behavior is simplified, and the telemetry broadcast flow is still fairly minimal compared to a complete ground-station experience.

## Good places to start reading
1. [src/main.cpp](src/main.cpp) — the composition root and server setup
2. [src/Services/MissionPlanner.cpp](src/Services/MissionPlanner.cpp) — the core mission dispatch flow
3. [src/Domain/Drone.cpp](src/Domain/Drone.cpp) — drone state logic
4. [src/Controller/MissionController.cpp](src/Controller/MissionController.cpp) — REST API behavior
5. [src/Controller/TelemetryWebSocketController.cpp](src/Controller/TelemetryWebSocketController.cpp) — WebSocket command flow

## Prompt for Claude
You are reviewing a C++17 drone bunker management prototype. Explain what this project is doing at this stage based on the repository files. Focus on the overall purpose, architecture, key classes, runtime flow, current capabilities, and any notable gaps or simplifications. Mention the main implemented features, how the REST and WebSocket interfaces work, and suggest the best order for reading the codebase for someone new to the project.

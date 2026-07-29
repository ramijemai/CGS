# CGS (Command & Ground Station) — Codebase Overview

## Summary
CGS is a **drone fleet management ground station** built in C++17. It manages a "bunker" — a physical or virtual docking station with multiple bay slots — that can house, charge, launch, and recover drones. The system exposes a REST API and a WebSocket endpoint via the [Crow](https://github.com/CrowCpp/Crow) HTTP library, serving an embedded single-page HTML dashboard for real-time monitoring and command dispatch.

The project simulates the full lifecycle of drone missions: pre-flight checks, launch, telemetry tracking, return-to-bunker (RTL), and auto-docking with hatch management. It is structured as a layered architecture with Domain objects, Service layer orchestration, and HTTP/WS Controllers.

## Architecture

The system follows a **layered architecture** with four tiers:

```
main.cpp (composition root)
  │
  ├── Services (orchestration & business logic)
  │   ├── CapacityEngine     — manages bay slot inventory
  │   ├── MissionPlanner     — selects drone, executes launch
  │   ├── RecoveryService    — handles RTL and auto-docking
  │   └── TelemetryManager   — tracks active drone telemetry
  │
  ├── Domain (entities)
  │   ├── Drone              — drone state, battery, location
  │   ├── BaySlot            — slot occupancy, hatch, charging
  │   └── Bunker             — GPS location of bunker
  │
  ├── Controllers (HTTP/WS request handling)
  │   ├── MissionController           — REST: bunker status, launch
  │   └── TelemetryWebSocketController — WS: commands & telemetry broadcast
  │
  └── Output / Dashboard
      ├── DashboardHtml.h    — full in-memory HTML+CSS+JS dashboard
      └── main.cpp via Crow  — serves static routes & WS
```

**Key patterns:**
- **Dependency injection via constructor references**: All services take their dependencies as references in the constructor (no DI framework).
- **Simulated hardware**: There is no actual drone hardware interface — the system simulates state transitions.
- **In-memory state**: No database. Everything lives in `std::vector<>` and `std::map<>` within the service objects.
- **Embedded frontend**: The entire dashboard is a raw string literal in `DashboardHtml.h`. No separate frontend build step.

The codebase uses **Crow** as the web framework. `main.cpp` runs `crow::SimpleApp` on port 18080, registers direct REST lambdas and a WebSocket route.

### Technology Stack
| Technology | Use |
|---|---|
| C++17 | Language standard |
| CMake ≥3.16 | Build system |
| Crow (header-only) | HTTP + WebSocket server |
| nlohmann/json (header-only) | JSON parsing/serialization |
| POSIX sockets (`sys/socket.h`, `netinet/in.h`) | WebSocketServer.cpp (raw socket impl) |
| `select()` | Event loop for the custom WebSocket server |

**Important duality**: There are **two** WebSocket paths in the codebase:
1. **Crow's built-in WebSocket** (via `crow::websocket`) — used in `main.cpp` for the `/ws/telemetry` route, handled by `TelemetryWebSocketController`.
2. **Custom raw-socket WebSocketServer** (`src/Services/WebSocketServer.cpp`) — a standalone TCP server with its own SHA-1 handshake, frame encode/decode, and `select()`-based event loop. It references a `Controller` class that **does not exist** in the codebase (more on this below).

## Directory Structure

```
CGS/
├── CMakeLists.txt               — CMake build (C++17, no external deps besides header-only libs)
├── inc/
│   ├── Common/
│   │   ├── DashboardHtml.h     — Full dashboard HTML/CSS/JS (inline string)
│   │   └── Types.h             — GpsCoordinate, DroneState, BayState, HatchState enums
│   ├── Controller/
│   │   ├── MissionController.h
│   │   └── TelemetryWebSocketController.h
│   ├── Domain/
│   │   ├── BaySlot.h
│   │   ├── Bunker.h
│   │   └── Drone.h
│   ├── DTOs/
│   │   └── MissionDtos.h       — LaunchMissionRequestDto, ApiResponseDto
│   └── Services/
│       ├── CapacityEngine.h
│       ├── MissionPlanner.h
│       ├── RecoveryService.h
│       └── TelemetryManager.h
├── src/
│   ├── Controller/
│   │   ├── MissionController.cpp
│   │   └── TelemetryWebSocketController.cpp
│   ├── Domain/
│   │   ├── BaySlot.cpp
│   │   ├── Bunker.cpp
│   │   └── Drone.cpp
│   ├── Services/
│   │   ├── CapacityEngine.cpp
│   │   ├── MissionPlanner.cpp
│   │   ├── RecoveryService.cpp
│   │   ├── TelemetryManager.cpp
│   │   └── WebSocketServer.cpp  — Custom raw-socket WS server (⚠️ separate from Crow WS)
│   └── main.cpp
└── build/
    └── BunkerProject            — compiled binary
```

## Key Abstractions

### `Drone`
- **File**: `inc/Domain/Drone.h` / `src/Domain/Drone.cpp`
- **Responsibility**: Represents a single drone with ID, battery (0-100%), GPS location, and state.
- **Key properties**: `m_id`, `m_batteryLevel` (clamped), `m_state` (DroneState enum), `m_currentLocation`
- **State machine**: Idle → Charging → Ready → InFlight → ReturningToBunker → Landing → Fault
- **Lifecycle**: Created manually in tests/simulation. Lives in `shared_ptr` ownership via BaySlot.
- **`isReadyForMission()`**: Returns true if state is Idle/Charging/Ready AND battery ≥ 90%.

### `BaySlot`
- **File**: `inc/Domain/BaySlot.h` / `src/Domain/BaySlot.cpp`
- **Responsibility**: A single docking bay — tracks occupancy, hatch state, and charging.
- **Key methods**: `dockDrone()`, `undockDrone()`, `chargeTick(double amountPercent)`
- **Charge logic**: Calling `chargeTick()` increases battery. When battery reaches ≥99%, the drone transitions to `Ready` state.
- **Docking precondition**: Bay must be `Vacant`. Docking sets drone to `Charging` state.

### `CapacityEngine`
- **File**: `inc/Services/CapacityEngine.h` / `src/Services/CapacityEngine.cpp`
- **Responsibility**: Owns the vector of BaySlots. Provides slot lookup and search.
- **Key methods**: `getSlot(int)`, `findReadyDroneSlot()`, `findVacantSlot()`, `getAllSlots()`
- **Linear scan**: All searches are O(n) loops over the slot vector.

### `MissionPlanner`
- **File**: `inc/Services/MissionPlanner.h` / `src/Services/MissionPlanner.cpp`
- **Responsibility**: Orchestrates drone launch. Finds a ready drone, runs pre-flight diagnostics, sets state to InFlight, and undocks it.
- **⚠️ Bug**: After calling `undockDrone()`, the slot is vacated but the drone is **not returned** from `undockDrone()` — the result is discarded. The drone pointer is still held by the `slot->getDrone()` local variable, but now the slot no longer owns it. This is a **use-after-free**: `getDrone()` returns `nullptr` after `undockDrone()` resets the shared_ptr. The local `drone` shared_ptr still exists (has the reference), so it works, but the slot is left in an inconsistent state.

### `RecoveryService`
- **File**: `inc/Services/RecoveryService.h` / `src/Services/RecoveryService.cpp`
- **Responsibility**: Handles return-to-bunker (RTL): finds vacant slot, opens hatch, sets drone to Landing, docks, closes hatch.
- **Edge case**: If no vacant slot is found, it prints a "HOLDING PATTERN" message and returns false.

### `TelemetryManager`
- **File**: `inc/Services/TelemetryManager.h` / `src/Services/TelemetryManager.cpp`
- **Responsibility**: Manages in-flight drone telemetry in a `map<string, DroneTelemetry>`. Supports registration, updates, command dispatch, history.
- **Notable**: `simulateDroneFlight()` is **commented out** in the `.cpp` file. Telemetry is not auto-updated — it requires explicit calls.
- **History**: `m_telemetryHistory` and `m_commandHistory` grow unboundedly (no limit).

### `MissionController`
- **File**: `inc/Controller/MissionController.h` / `src/Controller/MissionController.cpp`
- **Responsibility**: REST endpoint handler for `/api/v1/missions/launch` and `/api/v1/bunker/status`.
- **Launch parsing**: Expects JSON with `latitude`, `longitude`, `altitude`, `durationSeconds`. Returns 200/400.

### `TelemetryWebSocketController`
- **File**: `inc/Controller/TelemetryWebSocketController.h` / `src/Controller/TelemetryWebSocketController.cpp`
- **Responsibility**: Handles Crow WebSocket messages at `/ws/telemetry`. Parses JSON with `action`, `droneId`, `commandCode`.
- **Limitations**: Only handles `SEND_COMMAND` action. `broadcastTelemetry()` is a stub that sends a bare `{"event":"TELEMETRY_UPDATE"}` with no actual telemetry data.

### `WebSocketServer` (⚠️ UNUSED / UNREFERENCED)
- **File**: `src/Services/WebSocketServer.cpp`
- **Responsibility**: A standalone, raw-socket WebSocket server with built-in SHA-1 and frame encoding. Servers the dashboard HTML at `GET /` and WebSocket connections at any path.
- **⚠️ CRITICAL**: This file has no header (`inc/Services/WebSocketServer.h` does not exist). It references a `Controller` class (via `m_controller`) that **does not exist anywhere in the codebase**. The `Controller` type is used throughout this file (`Controller::handleCommand()`, `Controller::getSystemStateJson()`, `Controller::tick()`, `Controller::logEvent()`, `Controller::CommandResult`), but no `Controller.h` or `Controller.cpp` exists. This file will **not compile**.
- It is **not listed** in the `CMakeLists.txt` SOURCES list.

## Data Flow

### Mission Launch (REST)
1. Client `POST /api/v1/missions/launch` with `{latitude, longitude, altitude, durationSeconds}`
2. Crow lambda → `MissionController::handleLaunchMission()`
3. Parses JSON → builds `GpsCoordinate`
4. Calls `MissionPlanner::planAndExecuteInspection(target, duration)`
5. Planner calls `CapacityEngine::findReadyDroneSlot()` → linear scan
6. If found: checks Fault state, prints diagnostics, sets drone to `InFlight`, **undocks drone from slot** (discards returned pointer)
7. Returns `200` or `400` JSON

### Bunker Status (REST)
1. Client `GET /api/v1/bunker/status`
2. Crow lambda → `MissionController::handleGetBunkerStatus()`
3. Iterates all slots, builds JSON array with slot ID, occupancy, drone ID + battery
4. Returns `200` JSON

### WebSocket Telemetry (Crow WS)
1. Client connects to `ws://host:18080/ws/telemetry`
2. Crow's built-in WS → lambda that calls `wsController.onMessage(data, sendReply)`
3. `TelemetryWebSocketController::onMessage()` parses JSON, dispatches `SEND_COMMAND` to `TelemetryManager::sendCommand()`
4. No push telemetry is actually sent — `broadcastTelemetry()` is a stub

### WebSocketServer (raw socket — NOT connected in current main.cpp)
1. `WebSocketServer::start()` opens a TCP socket, binds, listens
2. `serverLoop()` uses `select()` with 1s timeout to multiplex listen + client sockets
3. New connections: if HTTP upgrade request → perform WS handshake → send initial state
4. WS frames decoded → JSON parsed → `m_controller.handleCommand(cmdType, droneId)` called
5. Response sent back + state broadcast to all clients
6. HTTP requests: serve dashboard HTML at `/`, state JSON at `/state`, command POST at `/cmd`

## Current Issues & Non-Obvious Behaviors

### Critical: WebSocketServer.cpp references a nonexistent `Controller` class
- The file uses `m_controller` as `Controller&`, calling `handleCommand()`, `getSystemStateJson()`, `tick()`, `logEvent()`
- No `Controller.h` or `Controller.cpp` exists in the codebase
- The file is also missing from CMakeLists.txt — it was likely in progress or recently removed

### Use-after-free in MissionPlanner
- `MissionPlanner::planAndExecuteInspection()` calls `slot->undockDrone()` but discards the returned `shared_ptr<Drone>`
- The local `drone` shared_ptr still holds a reference (since `slot->getDrone()` returned the same shared_ptr before undock), but after `undockDrone()` the slot's internal `m_drone.reset()` is called
- This works only because `drone` was captured before the undock call — if the code were reordered, it would break

### Dual WebSocket server confusion
- `main.cpp` uses **Crow's built-in WebSocket** on `/ws/telemetry`
- `WebSocketServer.cpp` implements a **separate, custom WebSocket server** on an independent port
- Neither is connected to the other. `WebSocketServer` is not instantiated or started anywhere in `main.cpp`.
- This is likely dead code or the start of a migration away from Crow.

### Stub telemetry broadcasting
- `TelemetryWebSocketController::broadcastTelemetry()` sends only `{"event":"TELEMETRY_UPDATE"}` with no actual data
- The dashboard expects a `state` JSON with `slots`, `activeDrones`, `commandLog`, etc. (see dashboard JS `renderBunker()`, `renderDrones()`, etc.)
- The Crow-based WS pathway will not provide the state the dashboard expects

### Embedded dashboard state shape
- The dashboard expects a specific JSON state shape: `{ slots: [{slotId, bayState, hatchState, droneId, batteryLevel}], activeDrones: [{droneId, latitude, longitude, altitude, batteryLevel, speed, heading}], commandLog: [...], activeDroneCount: N }`
- `MissionController::handleGetBunkerStatus()` returns a **different shape**: `{ slots: [{slotId, isOccupied, drone: {id, batteryLevel}}] }` — missing `bayState`, `hatchState`, `droneId` (flat), `batteryLevel` (flat)
- The dashboard will not render correctly with the current REST response

### In-memory unbounded growth
- `TelemetryManager` stores all commands and all telemetry snapshots in `std::vector<>` that grows indefinitely
- No pruning, no time-based cleanup

### Battery charging granularity
- `BaySlot::chargeTick()` accepts an arbitrary `double amountPercent` — no tick rate or time simulation
- The drone transitions to `Ready` at ≥99%, not 100%

### `Drone::isReadyForMission()` condition
- Requires battery ≥ 90% AND state to be `Idle`, `Charging`, or `Ready`
- If a drone is already `Ready`, it qualifies. If it's `Charging` at 90%, it qualifies (but may still be charging)

### Commented-out flight simulation
- `TelemetryManager::simulateDroneFlight()` is commented out — it would move a drone toward target and drain battery
- No flight physics or position interpolation exists elsewhere

### Include path discrepancy
- `main.cpp` includes `"Controllers/MissionController.h"` and `"Controllers/TelemetryWebSocketController.h"` (plural `Controllers`)
- The actual directory and files use `Controller` (singular)
- The build would fail with `fatal error: Controllers/MissionController.h: No such file or directory`

### Missing license file
- No LICENSE file found in the root

## Module Reference

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Build config — C++17, includes `inc/`, links source files into `BunkerProject` executable |
| `src/main.cpp` | Composition root: creates all services, controllers, Crow app, registers routes, starts on port 18080 |
| `inc/Common/Types.h` | Shared enums (`DroneState`, `BayState`, `HatchState`) and `GpsCoordinate` struct |
| `inc/Common/DashboardHtml.h` | Single-page HTML dashboard (inline string) with full CSS and JS WebSocket client |
| `inc/DTOs/MissionDtos.h` | `LaunchMissionRequestDto`, `ApiResponseDto` — used as documentation, not actually used in code |
| `inc/Domain/Drone.h` | Drone entity — id, battery, state, location |
| `src/Domain/Drone.cpp` | Drone implementation — clamped battery, `isReadyForMission()` check |
| `inc/Domain/BaySlot.h` | Bay slot — occupancy, hatch, charging |
| `src/Domain/BaySlot.cpp` | Dock/undock/charge logic — state transitions |
| `inc/Domain/Bunker.h` | Bunker entity — just stores a GPS location |
| `src/Domain/Bunker.cpp` | Bunker implementation — trivial get/set |
| `inc/Services/CapacityEngine.h` | Bay slot inventory — list of slots, find ready/vacant |
| `src/Services/CapacityEngine.cpp` | Creates N slots; linear search methods |
| `inc/Services/MissionPlanner.h` | Mission launch orchestration |
| `src/Services/MissionPlanner.cpp` | Pre-flight checks, dispatching, undock bug |
| `inc/Services/RecoveryService.h` | Return-to-bunker and docking |
| `src/Services/RecoveryService.cpp` | Hatch open, drone landing, dock, hatch close |
| `inc/Services/TelemetryManager.h` | DroneTelemetry struct, TelemetryMessage struct, TelemetryManager |
| `src/Services/TelemetryManager.cpp` | Active drone registry, telemetry updates, command history |
| `inc/Controller/MissionController.h` | REST handler for launch and status |
| `src/Controller/MissionController.cpp` | JSON parsing, calls MissionPlanner and CapacityEngine |
| `inc/Controller/TelemetryWebSocketController.h` | WS handler for drone commands |
| `src/Controller/TelemetryWebSocketController.cpp` | JSON parsing, dispatches to TelemetryManager |
| `src/Services/WebSocketServer.cpp` | **Dead code / in-progress**: custom raw WS server referencing nonexistent `Controller` |

## Suggested Reading Order

1. **`inc/Common/Types.h`** — foundational enums and data structures used everywhere
2. **`inc/Domain/Drone.h` + `src/Domain/Drone.cpp`** — the core entity and its state machine
3. **`inc/Domain/BaySlot.h` + `src/Domain/BaySlot.cpp`** — docking, charging, hatch state
4. **`inc/Services/CapacityEngine.h` + `src/Services/CapacityEngine.cpp`** — slot inventory, the "database"
5. **`inc/Services/MissionPlanner.h` + `src/Services/MissionPlanner.cpp`** — the primary orchestration flow
6. **`src/main.cpp`** — how everything wires together, the composition root

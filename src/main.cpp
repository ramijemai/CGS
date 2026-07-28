#include "Services/CapacityEngine.h"
#include "Services/MissionPlanner.h"
#include "Services/RecoveryService.h"
#include "Services/TelemetryManager.h"
#include "Domain/Drone.h"
#include <iostream>
#include <memory>

namespace {

void printBunkerStatus(const CapacityEngine& bunkerEngine) {
    std::cout << "\n[STATUS] Bunker slots:\n";
    for (const auto& slot : bunkerEngine.getAllSlots()) {
        std::cout << "  - Bay " << slot->getSlotId() << ": ";
        if (slot->isOccupied()) {
            const auto drone = slot->getDrone();
            std::cout << drone->getId() << " (" << drone->getBatteryLevel() << "% battery)\n";
        } else {
            std::cout << "[VACANT]\n";
        }
    }
}

} // namespace

int main() {
    CapacityEngine bunkerEngine(2);
    TelemetryManager telemetry;

    auto droneAlpha = std::make_shared<Drone>("DRONE-ALPHA", 100.0);
    auto droneBeta = std::make_shared<Drone>("DRONE-BETA", 95.0);

    bunkerEngine.getSlot(1)->dockDrone(droneAlpha);
    bunkerEngine.getSlot(2)->dockDrone(droneBeta);

    std::cout << "[BOOT] Bunker initialized with two drones.\n";
    printBunkerStatus(bunkerEngine);

    MissionPlanner planner(bunkerEngine);
    GpsCoordinate inspectionTarget{36.8065, 10.1815, 100.0};

    if (!planner.planAndExecuteInspection(inspectionTarget, 120.0)) {
        std::cerr << "[ERROR] Mission launch failed.\n";
        return 1;
    }

    telemetry.registerActiveDrone(droneAlpha, inspectionTarget);
    telemetry.printActiveDronesTelemetry();

    TelemetryMessage returnCommand;
    returnCommand.droneId = droneAlpha->getId();
    returnCommand.command = DroneCommand::RETURN_TO_BUNKER;
    returnCommand.messageText = "Mission complete, return to bunker";
    telemetry.sendCommand(droneAlpha->getId(), returnCommand);

    droneAlpha->setBatteryLevel(30.0);
    RecoveryService recoveryService(bunkerEngine);
    if (!recoveryService.executeRecoveryAndDocking(droneAlpha)) {
        std::cerr << "[ERROR] Recovery failed.\n";
        return 1;
    }

    telemetry.unregisterActiveDrone(droneAlpha->getId());
    printBunkerStatus(bunkerEngine);

    std::cout << "[DONE] Mission flow completed using the service layer.\n";
    return 0;
}

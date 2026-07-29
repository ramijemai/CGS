#include "Services/RecoveryService.h"
#include <iostream>

RecoveryService::RecoveryService(CapacityEngine& capacityEngine)
    : m_capacityEngine(capacityEngine) {}

bool RecoveryService::executeRecoveryAndDocking(std::shared_ptr<Drone> returningDrone, const Bunker& bunker) {
    if (!returningDrone) {
        std::cerr << "[RECOVERY ERROR] Null drone reference passed for recovery.\n";
        return false;
    }

    std::cout << "\n==================================================\n";
    std::cout << "     INITIATING PHASE 2: RECOVERY & AUTO-DOCK     \n";
    std::cout << "==================================================\n";

    std::cout << "[RECOVERY] Drone '" << returningDrone->getId() 
              << "' initiating RTL (Battery: " 
              << returningDrone->getBatteryLevel() << "%)...\n";

    const auto& bunkerLocation = bunker.getGpsLocation();
    std::cout << "[RECOVERY] Return target set to bunker GPS ("
              << bunkerLocation.latitude << ", " << bunkerLocation.longitude
              << ", " << bunkerLocation.altitude << "m).\n";
    
    returningDrone->setState(DroneState::ReturningToBunker);

    auto targetSlot = m_capacityEngine.findVacantSlot();
    if (!targetSlot) {
        std::cerr << "[RECOVERY CRITICAL] No vacant slots available! Holding pattern engaged.\n";
        return false;
    }

    std::cout << "[RECOVERY] Allocated Bay " << targetSlot->getSlotId() << " for recovery.\n";

    std::cout << "[RECOVERY] Opening hatch for Bay " << targetSlot->getSlotId() << "...\n";
    targetSlot->setHatchState(HatchState::Open);

    returningDrone->setState(DroneState::Landing);
    std::cout << "[RECOVERY] Drone on final approach... Touchdown confirmed.\n";

    targetSlot->dockDrone(returningDrone);
    targetSlot->setHatchState(HatchState::Closed);

    std::cout << "[RECOVERY SUCCESS] Drone '" << returningDrone->getId() 
              << "' safely docked in Bay " << targetSlot->getSlotId() 
              << ". Hatch CLOSED. Charging initialized.\n";

    return true;
}
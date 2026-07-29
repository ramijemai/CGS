#pragma once
#include "Services/CapacityEngine.h"
#include "Domain/Drone.h"
#include "Domain/Bunker.h"

class RecoveryService {
public:
    explicit RecoveryService(CapacityEngine& capacityEngine);

    bool executeRecoveryAndDocking(std::shared_ptr<Drone> returningDrone, const Bunker& bunker);

private:
    CapacityEngine& m_capacityEngine;
};
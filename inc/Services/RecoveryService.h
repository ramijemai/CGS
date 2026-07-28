#pragma once
#include "Services/CapacityEngine.h"
#include "Domain/Drone.h"

class RecoveryService {
public:
    explicit RecoveryService(CapacityEngine& capacityEngine);

    bool executeRecoveryAndDocking(std::shared_ptr<Drone> returningDrone);

private:
    CapacityEngine& m_capacityEngine;
};
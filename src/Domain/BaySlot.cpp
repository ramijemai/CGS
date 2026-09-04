#include "Domain/BaySlot.h"

BaySlot::BaySlot(int slotId)
    : m_slotId(slotId), m_bayState(BayState::Vacant), m_hatchState(HatchState::Closed) {}

int BaySlot::getSlotId() const {
    return m_slotId;
}

BayState BaySlot::getBayState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bayState;
}

HatchState BaySlot::getHatchState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hatchState;
}

bool BaySlot::isOccupied() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_drone != nullptr;
}

std::shared_ptr<Drone> BaySlot::getDrone() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_drone;
}

std::shared_ptr<Drone> BaySlot::getDroneIfOccupied() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_drone;   // null if vacant — same single locked read as getDrone(), but this is the name that signals "use this instead of isOccupied()+getDrone()"
}

void BaySlot::setHatchState(HatchState state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hatchState = state;
}

bool BaySlot::dockDrone(std::shared_ptr<Drone> drone, bool markCharging) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bayState == BayState::Occupied || !drone) return false;

    m_drone = drone;
    m_bayState = BayState::Occupied;

    if (markCharging || drone->getBatteryLevel() < 20.0) {
        drone->setState(DroneState::Charging);
    } else {
        drone->setState(DroneState::Docked);
    }
    return true;
}

std::shared_ptr<Drone> BaySlot::undockDrone() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_drone) return nullptr;

    auto drone = m_drone;
    m_drone.reset();
    m_bayState = BayState::Vacant;
    drone->setState(DroneState::Idle);
    return drone;
}

void BaySlot::chargeTick(double amountPercent) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_drone && m_drone->getState() == DroneState::Charging) {
        double newLevel = m_drone->getBatteryLevel() + amountPercent;
        m_drone->setBatteryLevel(newLevel);
        if (m_drone->getBatteryLevel() >= 99.0) {
            m_drone->setState(DroneState::Ready);
        }
    }
}
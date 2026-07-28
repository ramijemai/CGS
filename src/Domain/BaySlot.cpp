#include "Domain/BaySlot.h"

BaySlot::BaySlot(int slotId)
    : m_slotId(slotId), m_bayState(BayState::Vacant), m_hatchState(HatchState::Closed) {}

int BaySlot::getSlotId() const {
    return m_slotId;
}

BayState BaySlot::getBayState() const {
    return m_bayState;
}

HatchState BaySlot::getHatchState() const {
    return m_hatchState;
}

bool BaySlot::isOccupied() const {
    return m_drone != nullptr;
}

std::shared_ptr<Drone> BaySlot::getDrone() const {
    return m_drone;
}

void BaySlot::setHatchState(HatchState state) {
    m_hatchState = state;
}

bool BaySlot::dockDrone(std::shared_ptr<Drone> drone) {
    if (m_bayState == BayState::Occupied || !drone) return false;
    m_drone = drone;
    m_bayState = BayState::Occupied;
    m_drone->setState(DroneState::Charging);
    return true;
}

std::shared_ptr<Drone> BaySlot::undockDrone() {
    if (!m_drone) return nullptr;
    auto drone = m_drone;
    m_drone.reset();
    m_bayState = BayState::Vacant;
    return drone;
}

void BaySlot::chargeTick(double amountPercent) {
    if (m_drone && m_drone->getState() == DroneState::Charging) {
        double newLevel = m_drone->getBatteryLevel() + amountPercent;
        m_drone->setBatteryLevel(newLevel);
        if (m_drone->getBatteryLevel() >= 99.0) {
            m_drone->setState(DroneState::Ready);
        }
    }
}
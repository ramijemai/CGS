#include "Services/CapacityEngine.h"

CapacityEngine::CapacityEngine(size_t totalSlots) {
    for (size_t i = 0; i < totalSlots; ++i) {
        m_slots.push_back(std::make_shared<BaySlot>(static_cast<int>(i + 1)));
    }
}

std::shared_ptr<BaySlot> CapacityEngine::getSlot(int slotId) {
    for (auto& slot : m_slots) {
        if (slot->getSlotId() == slotId) return slot;
    }
    return nullptr;
}

std::shared_ptr<BaySlot> CapacityEngine::findReadyDroneSlot() {
    for (auto& slot : m_slots) {
        if (slot->isOccupied() && slot->getDrone()->isReadyForMission()) {
            return slot;
        }
    }
    return nullptr;
}

std::shared_ptr<BaySlot> CapacityEngine::findVacantSlot() {
    for (auto& slot : m_slots) {
        if (!slot->isOccupied() && slot->getBayState() == BayState::Vacant) {
            return slot;
        }
    }
    return nullptr;
}

const std::vector<std::shared_ptr<BaySlot>>& CapacityEngine::getAllSlots() const {
    return m_slots;
}
#pragma once
#include "Domain/BaySlot.h"
#include "Common/Types.h"
#include <vector>
#include <memory>
#include <cstddef>

class CapacityEngine {
public:

    

    explicit CapacityEngine(size_t totalSlots);

    std::shared_ptr<BaySlot> getSlot(int slotId);
    std::shared_ptr<BaySlot> findReadyDroneSlot();
    std::shared_ptr<BaySlot> findVacantSlot();
    const std::vector<std::shared_ptr<BaySlot>>& getAllSlots() const;

private:
    std::vector<std::shared_ptr<BaySlot>> m_slots;
};
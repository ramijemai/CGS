#pragma once
#include "Domain/Drone.h"
#include "Common/Types.h"
#include <memory>

class BaySlot {
public:
    explicit BaySlot(int slotId);

    int getSlotId() const;
    BayState getBayState() const;
    HatchState getHatchState() const;
    bool isOccupied() const;

    std::shared_ptr<Drone> getDrone() const;

    void setHatchState(HatchState state);
    bool dockDrone(std::shared_ptr<Drone> drone);
    std::shared_ptr<Drone> undockDrone();
    void chargeTick(double amountPercent);

private:
    int m_slotId;
    BayState m_bayState;
    HatchState m_hatchState;
    std::shared_ptr<Drone> m_drone{nullptr};
};
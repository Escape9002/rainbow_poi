#pragma once

#include <HardwareStates/HardwareState.h>

class Idle : public HardwareState
{
private:
    static constexpr uint32_t NO_MOTION_TIMEOUT = 1000 * 60 * 5;
    static constexpr uint32_t DEADZONE = 1000;

public:
    Idle(HAL *hal) : HardwareState(hal) {}

    HARDWARE_STATE execute(uint32_t value, uint32_t dt_ms) override
    {
        this->accumulated_time += dt_ms;

        if (this->accumulated_time > NO_MOTION_TIMEOUT)
        {
            return HARDWARE_STATE::SLEEP;
        }

        if (value > DEADZONE)
        {
            return HARDWARE_STATE::ON;
        }

        return HARDWARE_STATE::IDLE;
    }

    HARDWARE_STATE getState() override {
        return HARDWARE_STATE::IDLE;
    }
};
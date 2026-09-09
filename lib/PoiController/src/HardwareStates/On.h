#pragma once

#include <HardwareStates/HardwareState.h>

class On : public HardwareState
{
private:
    static constexpr uint32_t DEADZONE = 1000;

public:
    On(HAL *hal, EffectEngine *engine) : HardwareState(hal, engine) {}

    HARDWARE_STATE execute(uint32_t value, uint32_t dt_ms) override
    {

        if (value < DEADZONE)
        {
            return HARDWARE_STATE::IDLE;
        }

        return HARDWARE_STATE::ON;
    }

    HARDWARE_STATE getState() override {
        return HARDWARE_STATE::ON;
    }
};
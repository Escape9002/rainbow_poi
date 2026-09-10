#pragma once

#include <HardwareStates/HardwareState.h>

class Sleep : public HardwareState
{

public:
    Sleep(HAL *hal) : HardwareState(hal) {}

    HARDWARE_STATE execute(uint32_t value, uint32_t dt_ms) override
    {

        this->hal->enterDeepSleep();

        // this is not going to run, enterDeepSleep kills anything asap
        return HARDWARE_STATE::SLEEP;
    }

    HARDWARE_STATE getState() override {
        return HARDWARE_STATE::SLEEP;
    }
};
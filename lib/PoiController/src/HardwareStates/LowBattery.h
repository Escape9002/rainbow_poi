#pragma once

#include <HardwareStates/HardwareState.h>

class LowBattery : public HardwareState
{
private:
public:
    LowBattery(HAL *hal) : HardwareState(hal) {}

    HARDWARE_STATE execute(uint32_t value, uint32_t dt_ms) override
    {
        /**
         * > How do we get the battery state into this part of the code?
         * 
         * If we reach low battery, this should be done:
         * 1. enter the flashing mode of the animation-stack. It should flash red.
         *      Is handled by the poi-Controller, just ensure that we return the correct 
         *      Hardware State, so the controller knows whats happening.
         * 2. if smart(?) disable bluetooth.
         *      Should be a function in the HAL? I dont know how to disable bluetooth at runtime 
         *      at the moment.
         * 3. Enter deep-sleep if battery is nearing a very low number.
         *      This is a certified HAL moment, we can do this here!
         */

         
    }

    HARDWARE_STATE getState() override {
        return HARDWARE_STATE::LOW_BATTERY;
    }
};
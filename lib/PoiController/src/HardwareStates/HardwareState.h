#pragma once
#include "cstdint"
#include <HAL.h>
#include <EffectEngine.h>
#include <HardwareStates/HardwareStates.h>

class HardwareState
{
protected:
    uint32_t accumulated_time = 0;
    HAL *hal;

    static const uint8_t LOW_BATTERY = 10;

    virtual HARDWARE_STATE execute(uint32_t value, uint32_t dt_ms) = 0;

public:
    HardwareState(HAL *hal) : hal(hal)
    {
    }

    virtual ~HardwareState() = default;
    virtual void onEnter()
    {

        accumulated_time = 0;
    }
    /**
     * @brief compute "time" step in the controller
     *
     * @param value ensure that this value is normalized, such that theres no difference between different sensors.
     * @param dt_ms time since last call/ loop
     * @param batteryPercentage always check the battery percentage before calling the normal hardware checks
     */
    HARDWARE_STATE tick(uint32_t value, uint32_t dt_ms, uint8_t batteryPercentage)
    {

        if (batteryPercentage < LOW_BATTERY)
        {
            return HARDWARE_STATE::LOW_BATTERY;
        }

        execute(value, dt_ms);
    }

    virtual HARDWARE_STATE getState() = 0;
};
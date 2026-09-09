#pragma once
#include "cstdint"
#include <HAL.h>
#include <EffectEngine.h>

// would it be safer to use enum classes here?
// its realistic to run into name conflicts with words like 
// sleep, idle, etc.
enum HARDWARE_STATE
{
    SLEEP,
    IDLE,
    ON,
    LOW_BATTERY

};

class HardwareState
{
protected:
    uint32_t accumulated_time = 0;
    HAL *hal;
    EffectEngine *engine;

    

public:
    HardwareState(HAL *hal, EffectEngine *engine) : hal(hal), engine(engine)
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
     */
    virtual HARDWARE_STATE execute(uint32_t value, uint32_t dt_ms) = 0;
    virtual HARDWARE_STATE getState() = 0;
};
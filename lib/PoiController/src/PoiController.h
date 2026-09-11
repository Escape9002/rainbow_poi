#pragma once
#include <cstdint>

#include <LowPass.h>
#include <HSV.h>
#include <EffectEngine.h>
#include <HAL.h>
#include "AnimationState/AnimationState.h"
#include "AnimationState/Acceleration.h"
#include "AnimationState/Constant.h"
#include "AnimationState/Gyro.h"
#include "AnimationState/Rainbow.h"
#include "AnimationState/Flash.h"

#include "HardwareStates/HardwareState.h"
#include "HardwareStates/Sleep.h"
#include "HardwareStates/Idle.h"
#include "HardwareStates/LowBattery.h"
#include "HardwareStates/On.h"

class PoiController
{
private:
    // --------------------------------------------------------
    // Controller objects
    // --------------------------------------------------------
    EffectEngine &effectEngine;
    HAL &hal;
    uint8_t batteryPercentage = 100;

    // --------------------------------------------------------
    // Animation States
    // --------------------------------------------------------
    uint32_t NORM_SCALE = 1000;

    AcclAnimationState acclAni;
    ConstantAnimationState constAni;
    FlashAnimationState flashAni;
    GyroAnimationState gyroAni;
    RainbowAnimationState rainbowAni;

    ANIMATION_STATE animState = ANIMATION_STATE::ACCL;

    // --------------------------------------------------------
    // Hardware states
    // --------------------------------------------------------

    Sleep sleepState;
    On onState;
    Idle idleState;
    LowBattery lowBatteryState;

    HARDWARE_STATE hardwareState = HARDWARE_STATE::IDLE;

    // --------------------------------------------------------
    // Helper Functions
    // --------------------------------------------------------

    AnimationState *getAnimator()
    {
        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            return &acclAni;
        case ANIMATION_STATE::GYRO:
            return &gyroAni;
        case ANIMATION_STATE::CONST:
            return &constAni;
        case ANIMATION_STATE::FLASH:
            return &flashAni;
        case ANIMATION_STATE::RAINBOW:
            return &rainbowAni;
        }

        __builtin_unreachable();
    }

    void enterAnimationState()
    {
        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            acclAni.onEnter();
            break;

        case ANIMATION_STATE::CONST:
            constAni.onEnter();
            break;

        case ANIMATION_STATE::FLASH:
            flashAni.onEnter();
            break;

        case ANIMATION_STATE::GYRO:
            gyroAni.onEnter();
            break;

        case ANIMATION_STATE::RAINBOW:
            rainbowAni.onEnter();
            break;
        }
    }

    void enterHardwareState()
    {
        switch (hardwareState)
        {
        case HARDWARE_STATE::IDLE:
            idleState.onEnter();
            break;

        case HARDWARE_STATE::ON:
            onState.onEnter();
            break;

        case HARDWARE_STATE::SLEEP:
            sleepState.onEnter();
            break;

        case HARDWARE_STATE::LOW_BATTERY:
            lowBatteryState.onEnter();
            break;
        }
    }

    // --------------------------------------------------------
    // dedicated Tick functions
    // --------------------------------------------------------

    HSV animationTick(uint32_t value, uint32_t dt_ms)
    {
        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            return acclAni.tick(value, dt_ms, &effectEngine);

        case ANIMATION_STATE::CONST:
            return constAni.tick(value, dt_ms, &effectEngine);

        case ANIMATION_STATE::FLASH:
            return flashAni.tick(value, dt_ms, &effectEngine);

        case ANIMATION_STATE::GYRO:
            return gyroAni.tick(value, dt_ms, &effectEngine);

        case ANIMATION_STATE::RAINBOW:
            return rainbowAni.tick(value, dt_ms, &effectEngine);
        }

        __builtin_unreachable();
    }

    void hardwareTick(int32_t value, uint32_t dt_ms)
    {
        HARDWARE_STATE newState;

        switch (hardwareState)
        {
        case HARDWARE_STATE::IDLE:
            newState = idleState.tick(value, dt_ms, batteryPercentage);
            break;

        case HARDWARE_STATE::ON:
            newState = onState.tick(value, dt_ms, batteryPercentage);
            break;

        case HARDWARE_STATE::SLEEP:
            newState = sleepState.tick(value, dt_ms, batteryPercentage);
            break;

        case HARDWARE_STATE::LOW_BATTERY:
            // if we are low on battery, we should flash red!
            if (animState != ANIMATION_STATE::FLASH){
                setAnimationState(ANIMATION_STATE::FLASH);
            }
            newState = lowBatteryState.tick(value, dt_ms, batteryPercentage);
            break;
        }

        if (newState != hardwareState)
        {
            hardwareState = newState;
            enterHardwareState();
        }
    }

public:
    /**
     * @brief Construct a new Poi Controller object
     *
     * @param accl_max maximum accleration
     * @param norm_scale scale on which to operate concerning float to fix-point
     * @param alpha lowPass alpha
     * @param min hueMin
     * @param max hueMax
     * @param dynamic_max enable dynamic maximum acceleration
     */
    PoiController(
        EffectEngine &engine,
        HAL &hal)
        : effectEngine(engine),
          hal(hal),
          acclAni(NORM_SCALE, 80, 0, 250, true, &effectEngine),
          constAni(NORM_SCALE, 80, 0, 250, true, &effectEngine),
          flashAni(NORM_SCALE, 80, 0, 250, true, &effectEngine),
          gyroAni(NORM_SCALE, 80, 0, 250, true, &effectEngine),
          rainbowAni(NORM_SCALE, 80, 0, 250, true, &effectEngine),
          onState(&hal),
          idleState(&hal),
          lowBatteryState(&hal),
          sleepState(&hal)
    {
        enterAnimationState();
        enterHardwareState();
    }

    // --------------------------------------------------------
    // general tick for outside world
    // --------------------------------------------------------

    HSV tick(uint32_t value, uint32_t dt_ms)
    {
        hardwareTick(value, dt_ms);

        return animationTick(value, dt_ms);
    }

    // --------------------------------------------------------
    // Getter + Setter
    // --------------------------------------------------------

    HARDWARE_STATE getHardwareState()
    {
        return hardwareState;
    }

    ANIMATION_STATE getAnimationState()
    {
        return animState;
    }

    void setAnimationState(ANIMATION_STATE newState)
    {
        animState = newState;
        enterAnimationState();
    }

    void setBatteryLevel(uint8_t newBatteryPercentage)
    {
        batteryPercentage = newBatteryPercentage;
    }

    void setAlpha(uint32_t alpha)
    {

        getAnimator()->setAlpha(alpha);
    }

    uint32_t getAlpha()
    {
        return getAnimator()->getAlpha();
    }

    void setColorRange(uint32_t min, uint32_t max)
    {
        getAnimator()->setColorRange(min, max);
    }

    uint32_t getHueMin()
    {
        return getAnimator()->getHueMin();
    }
    uint32_t getHueMax()
    {
        return getAnimator()->getHueMax();
    }

    void setDynamicMax(bool state)
    {
        getAnimator()->setDynamicMax(state);
    }
};
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
    // Normal Vars
    // --------------------------------------------------------
    EffectEngine &effectEngine;
    HAL &hal;
    uint8_t batteryPercentage = 100;

    // --------------------------------------------------------
    // Animation State vars
    // --------------------------------------------------------
    // All of these States are controller specific, I dont think the
    // main.c should have to know of any of these.
    uint32_t NORM_SCALE = 1000;

    AcclAnimationState acclAni;
    ConstantAnimationState constAni;
    FlashAnimationState flashAni;
    GyroAnimationState gyroAni;
    RainbowAnimationState rainbowAni;

    ANIMATION_STATE animState = ANIMATION_STATE::ACCL;

    // --------------------------------------------------------
    // Hardware state vars
    // --------------------------------------------------------
    // All of these States are controller specific, I dont think the
    // main.c should have to know of any of these.

    Sleep sleepState;
    On onState;
    Idle idleState;
    LowBattery lowBatteryState;

    HARDWARE_STATE hardwareState = HARDWARE_STATE::IDLE;

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

    HSV tick(uint32_t value, uint32_t dt_ms)
    {
        hardwareTick(value, dt_ms);

        return animationTick(value, dt_ms);
    }

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
            newState = idleState.execute(value, dt_ms);
            break;

        case HARDWARE_STATE::ON:
            newState = onState.execute(value, dt_ms);
            break;

        case HARDWARE_STATE::SLEEP:
            newState = sleepState.execute(value, dt_ms);
            break;

        case HARDWARE_STATE::LOW_BATTERY:
            newState = lowBatteryState.execute(value, dt_ms);
            break;
        }

        if (newState != hardwareState)
        {
            hardwareState = newState;
            enterHardwareState();
        }
    }

    void setBatteryLevel(uint8_t newBatteryPercentage)
    {
        batteryPercentage = newBatteryPercentage;
    }

    //////////////////////////////////////////////////////////
    /// The below feels a little wrong, since we are directly
    /// passing the calls through to the underlying state.
    /// Doesnt this violtate some rule?
    /// > Jede Ebene muss etwas tun, sonst schlechtes Design?
    //////////////////////////////////////////////////////////

    void setAlpha(uint32_t alpha)
    {

        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            acclAni.setAlpha(alpha);
            break;

        case ANIMATION_STATE::CONST:
            constAni.setAlpha(alpha);
            break;

        case ANIMATION_STATE::FLASH:
            flashAni.setAlpha(alpha);
            break;

        case ANIMATION_STATE::GYRO:
            gyroAni.setAlpha(alpha);
            break;

        case ANIMATION_STATE::RAINBOW:
            rainbowAni.setAlpha(alpha);
            break;
        }
    }

    uint32_t getAlpha()
    {
        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            return acclAni.getAlpha();
            break;

        case ANIMATION_STATE::CONST:
            return constAni.getAlpha();
            break;

        case ANIMATION_STATE::FLASH:
            return flashAni.getAlpha();
            break;

        case ANIMATION_STATE::GYRO:
            return gyroAni.getAlpha();
            break;

        case ANIMATION_STATE::RAINBOW:
            return rainbowAni.getAlpha();
            break;
        }
    }

    void setColorRange(uint32_t min, uint32_t max)
    {
        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            acclAni.setColorRange(min, max);
            break;

        case ANIMATION_STATE::CONST:
            constAni.setColorRange(min, max);
            break;

        case ANIMATION_STATE::FLASH:
            flashAni.setColorRange(min, max);
            break;

        case ANIMATION_STATE::GYRO:
            gyroAni.setColorRange(min, max);
            break;

        case ANIMATION_STATE::RAINBOW:
            rainbowAni.setColorRange(min, max);
            break;
        }
    }

    uint32_t getHueMin()
    {
        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            acclAni.getHueMin();
            break;

        case ANIMATION_STATE::CONST:
            constAni.getHueMin();
            break;

        case ANIMATION_STATE::FLASH:
            flashAni.getHueMin();
            break;

        case ANIMATION_STATE::GYRO:
            gyroAni.getHueMin();
            break;

        case ANIMATION_STATE::RAINBOW:
            rainbowAni.getHueMin();
            break;
        }
    }
    uint32_t getHueMax()
    {
        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            acclAni.getHueMax();
            break;

        case ANIMATION_STATE::CONST:
            constAni.getHueMax();
            break;

        case ANIMATION_STATE::FLASH:
            flashAni.getHueMax();
            break;

        case ANIMATION_STATE::GYRO:
            gyroAni.getHueMax();
            break;

        case ANIMATION_STATE::RAINBOW:
            rainbowAni.getHueMax();
            break;
        }
    }

    void setDynamicMax(bool state)
    {
        switch (animState)
        {
        case ANIMATION_STATE::ACCL:
            acclAni.setDynamicMax(state);
            break;

        case ANIMATION_STATE::CONST:
            constAni.setDynamicMax(state);
            break;

        case ANIMATION_STATE::FLASH:
            flashAni.setDynamicMax(state);
            break;

        case ANIMATION_STATE::GYRO:
            gyroAni.setDynamicMax(state);
            break;

        case ANIMATION_STATE::RAINBOW:
            rainbowAni.setDynamicMax(state);
            break;
        }
    }
};
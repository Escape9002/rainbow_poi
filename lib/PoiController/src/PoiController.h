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
    EffectEngine *effectEngine;
    HAL *hal;
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

    AnimationState *animState = nullptr;

    // --------------------------------------------------------
    // Hardware state vars
    // --------------------------------------------------------
    // All of these States are controller specific, I dont think the
    // main.c should have to know of any of these.

    Sleep sleepState;
    On onState;
    Idle idleState;
    LowBattery lowBatteryState;

    HardwareState *hardwareState = nullptr;

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
        : effectEngine(&engine),
          hal(&hal),
          acclAni(NORM_SCALE, 80, 0, 250, true, effectEngine),
          constAni(NORM_SCALE, 80, 0, 250, true, effectEngine),
          flashAni(NORM_SCALE, 80, 0, 250, true, effectEngine),
          gyroAni(NORM_SCALE, 80, 0, 250, true, effectEngine),
          rainbowAni(NORM_SCALE, 80, 0, 250, true, effectEngine),
          onState(&hal),
          idleState(&hal),
          lowBatteryState(&hal),
          sleepState(&hal),
          animState(&acclAni),
          hardwareState(&idleState)
    {
        animState->onEnter();
        hardwareState->onEnter();
    }

    HSV tick(uint32_t value, uint32_t dt_ms)
    {
        HARDWARE_STATE newState = hardwareTick(value, dt_ms);
        if (newState != hardwareState->getState())
        {
            setHardwareState(newState);
        }

        return animationTick(value, dt_ms);
    }

    void setHardwareState(HARDWARE_STATE newState)
    {
        if (newState == hardwareState->getState())
        {
            // early return if now hardware state change happened
            return;
        }

        switch (newState)
        {
        case HARDWARE_STATE::IDLE:
            hardwareState = &idleState;
            break;
        case HARDWARE_STATE::LOW_BATTERY:
            hardwareState = &lowBatteryState;
            break;
        case HARDWARE_STATE::ON:
            hardwareState = &onState;
            break;
        case HARDWARE_STATE::SLEEP:
            hardwareState = &sleepState;
            break;
        default:
            // do nothing default
            return;
        }

        hardwareState->onEnter();
    }

    HARDWARE_STATE getHardwareState()
    {
        return hardwareState->getState();
    }

    void setAnimationState(ANIMATION_STATE newState)
    {
        if (animState->getState() == newState || newState == ANIMATION_STATE::ERROR)
        {
            // early return if no state change happens or an error was received
            return;
        }

        switch (newState)
        {
        case ANIMATION_STATE::ACCL:
            animState = &acclAni;
            break;

        case ANIMATION_STATE::ERROR:
        default:
            animState = &acclAni;
            break;
        }

        animState->onEnter();
    }

    ANIMATION_STATE getAnimationState()
    {
        return animState->getState();
    }

    std::string getAnimationStateStr()
    {
        switch (animState->getState())
        {
        case ANIMATION_STATE::ACCL:
            return "ACCL";
            break;

        default:
            break;
        }
    }

    HSV animationTick(uint32_t abs_value, uint32_t dt_ms)
    {
        return animState->tick(abs_value, dt_ms, effectEngine);
    }

    HARDWARE_STATE hardwareTick(int32_t value, uint32_t dt_ms)
    {
        return hardwareState->tick(value, dt_ms, batteryPercentage);
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
        this->animState->setAlpha(alpha);
    }

    uint32_t getAlpha()
    {
        return this->animState->getAlpha();
    }

    void setColorRange(uint32_t min, uint32_t max)
    {
        this->animState->setColorRange(min, max);
    }
    uint32_t getHueMin()
    {
        return this->animState->getHueMin();
    }
    uint32_t getHueMax()
    {
        return this->animState->getHueMax();
    }

    void setDynamicMax(bool state)
    {
        this->animState->setDynamicMax(state);
    }
};
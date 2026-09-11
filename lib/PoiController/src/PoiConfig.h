#pragma once
#include <cstdint>
#include "AnimationState/AnimationStates.h"

struct SensorConfig
{
    uint32_t alpha;
    uint16_t hueMin;
    uint16_t hueMax;
};

struct PoiConfig
{
    SensorConfig accl;
    SensorConfig gyro;
    ANIMATION_STATE animState;

    void update(SensorConfig newConfig, ANIMATION_STATE state)
    {
        if (state == ANIMATION_STATE::ACCL)
        {
            accl = newConfig;
        }

        if (state == ANIMATION_STATE::GYRO)
        {
            gyro = newConfig;
        }
    }

    std::string toString() const {
    return std::to_string(accl.alpha) + "," +
           std::to_string(accl.hueMin) + "," +
           std::to_string(accl.hueMax) + "," +
           std::to_string(gyro.alpha) + "," +
           std::to_string(gyro.hueMin) + "," +
           std::to_string(gyro.hueMax);
}

};

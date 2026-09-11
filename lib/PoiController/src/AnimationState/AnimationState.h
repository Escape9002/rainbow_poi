#pragma once
#include <HSV.h>
#include <EffectEngine.h>
#include <LowPass.h>
#include "AnimationStates.h"

class AnimationState
{
protected:
    const uint32_t NORM_SCALE = 1000;
    LowPassFilter<uint32_t> lPass;
    HSV hsv;
    uint16_t hueMin, hueMax;

    EffectEngine *engine;

    bool dynamic_max = false;

public:
    virtual ~AnimationState() = default;
    virtual void onEnter() = 0;
    virtual HSV tick(uint32_t value, uint32_t dt_ms, EffectEngine *effects) = 0;
    virtual ANIMATION_STATE getState() = 0;

    AnimationState(uint32_t norm_scale,
                   uint32_t alpha,
                   uint16_t hueMin,
                   uint16_t hueMax,
                   bool dynamic_max,
                   EffectEngine *engine)
        : NORM_SCALE(norm_scale),
          lPass(alpha),
          hueMin(hueMin),
          hueMax(hueMax),
          dynamic_max(dynamic_max),
          engine(engine)
    {
        hsv = {255, 255, 255};
    }

    int32_t normalize(int32_t value, int32_t value_max)
    {
        int32_t norm = (value * NORM_SCALE) / value_max;

        if (norm > NORM_SCALE)
        {
            norm = NORM_SCALE;
        }
        else if (norm < 0)
        {
            norm = 0;
        }

        return norm;
    }

    uint32_t filter(const uint32_t value)
    {
        return lPass.filter(value);
    }

    HSV map_color(const uint32_t value)
    {
        uint8_t hue = static_cast<uint8_t>((static_cast<uint32_t>(hsv.hueMapper(hueMin, hueMax, value)) * 255) / 360);

        return HSV{
            hue,
            255,
            255};
    }

    HSV animate(uint32_t max_variable, uint32_t &dynamic_max_variable, int32_t value)
    {
        if (dynamic_max)
        {
            if (value > max_variable && value > dynamic_max_variable)
            {
                dynamic_max_variable = value;
            }
            else if((dynamic_max_variable - 9) > max_variable)
            {
                dynamic_max_variable -= 10;
            }
        }

        uint32_t normalized = normalize(value, dynamic_max_variable);
        uint32_t filtered = filter(normalized);

        return map_color(filtered);
    }

    void setAlpha(uint32_t alpha)
    {
        lPass.setAlpha(alpha);
    }
    uint32_t getAlpha()
    {
        return lPass.getAlpha();
    }

    void setColorRange(uint32_t min, uint32_t max)
    {
        hueMax = max;
        hueMin = min;
    }
    uint16_t getHueMin()
    {
        return hueMin;
    }
    uint16_t getHueMax()
    {
        return hueMax;
    }

    void setDynamicMax(bool state)
    {
        dynamic_max = state;
    }
};
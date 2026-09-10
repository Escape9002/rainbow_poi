#pragma once
#include <AnimationState/AnimationState.h>

class RainbowAnimationState : public AnimationState
{
private:
    const uint8_t CYCLES_PER_MINUTE = 30;

public:
    RainbowAnimationState(uint32_t norm_scale,
                          uint32_t alpha,
                          uint16_t hueMin,
                          uint16_t hueMax,
                          bool dynamic_max,
                        EffectEngine* engine)
        : AnimationState(norm_scale, alpha, hueMin, hueMax, true, engine)
    {
    }

    void onEnter() override
    {
    }

    HSV tick(uint32_t absAccl, uint32_t dt_ms, EffectEngine *effectEngine) override
    {

        return engine->rainbow(CYCLES_PER_MINUTE);
    }

    ANIMATION_STATE getState() override
    {
        return ANIMATION_STATE::RAINBOW;
    }
};
#pragma once
#include <AnimationState/AnimationState.h>

class FlashAnimationState : public AnimationState
{
private:
    HSV hsv = HSV{255, 255, 255};
    const uint16_t INTERVAL = 500;

public:
    FlashAnimationState(uint32_t norm_scale,
                        uint32_t alpha,
                        uint16_t hueMin,
                        uint16_t hueMax,
                        bool dynamic_max,
                        EffectEngine *engine)
        : AnimationState(norm_scale, alpha, hueMin, hueMax, true, engine)
    {
    }

    void onEnter() override
    {
    }

    HSV tick(uint32_t absAccl, uint32_t dt_ms, EffectEngine *effectEngine) override
    {

        return engine->flash(hsv, INTERVAL);
    }

    void setHSV(HSV newHsv)
    {
        hsv = newHsv;
    }

    ANIMATION_STATE getState() override
    {
        return ANIMATION_STATE::CONST;
    }
};
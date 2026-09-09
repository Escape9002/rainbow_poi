#pragma once
#include <AnimationState/AnimationState.h>

class AcclAnimationState : public AnimationState
{
private:
    uint32_t intro_timer_ms = 0;
    const uint32_t INTRO_DURATION = 500;

    const uint32_t max_accl = 18000;
    uint32_t dynamic_max_var = max_accl;

public:
    AcclAnimationState(uint32_t norm_scale,
                       uint32_t alpha,
                       uint16_t hueMin,
                       uint16_t hueMax,
                       bool dynamic_max)
        : AnimationState(norm_scale, alpha, hueMin, hueMax, true)
    {
    }

    void onEnter() override
    {
        intro_timer_ms = INTRO_DURATION;
        dynamic_max_var = max_accl;
    }

    HSV tick(uint32_t absAccl, uint32_t dt_ms, EffectEngine *effectEngine) override
    {
        if (intro_timer_ms > 0)
        {
            if (dt_ms >= intro_timer_ms)
                intro_timer_ms = 0;
            else
                intro_timer_ms -= dt_ms;

            return effectEngine->flash(HSV{255, 255, 255}, 100);
        }

        return animate(max_accl, dynamic_max_var, absAccl);
    }

    void setMaxAccl(uint32_t absAccl)
    {
        dynamic_max_var = absAccl;
        dynamic_max = true;
    }

    ANIMATION_STATE getState() override {
        return ANIMATION_STATE::ACCL;
    }
};
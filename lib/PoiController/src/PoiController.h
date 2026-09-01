#include <cstdint>

#include <LowPass.h>
#include <HSV.h>

class PoiController
{
private:
    const uint32_t VALUE_MAX = 1000;
    uint32_t value_max_dyn = VALUE_MAX;
    bool dynamic_max = false;

    const uint32_t NORM_SCALE = 1000;

    uint32_t min, max;

    LowPass lPass;
    HSV hsv;

    int32_t normalize(int32_t value)
    {
        int32_t norm = (value * NORM_SCALE) / value_max_dyn;

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

    int32_t filter(int32_t value)
    {
        return lPass.filter(value);
    }

    HSV map_color(int32_t value)
    {
        return HSV{
            hsv.hueMapper(min, max, value),
            255,
            255};
    }

public:
    PoiController(uint32_t value_max,
                  uint32_t norm_scale,
                  uint32_t alpha,
                  uint32_t min,
                  uint32_t max,
                  bool dynamic_max)
        : VALUE_MAX(value_max),
          NORM_SCALE(norm_scale),
          min(min),
          max(max),
          dynamic_max(dynamic_max),
          lPass(alpha)

    {
        hsv = HSV{255, 255, 255};
    }

    HSV tick(int32_t value)
    {

        if (dynamic_max)
        {
            if (value > VALUE_MAX)
            {
                value_max_dyn = value;
            }
            else if (value_max_dyn > VALUE_MAX)
            {
                value_max_dyn -= (NORM_SCALE / VALUE_MAX);
            }
        }

        uint32_t normalized = normalize(value);
        uint32_t filtered = filter(normalized);

        return map_color(filtered);
    }

    void setAlpha(uint32_t alpha)
    {
        this->lPass.setAlpha(alpha);
    }

    void setColorRange(uint32_t min, uint32_t max)
    {
        this->min = min;
        this->max = max;
    }

    void setDynamicMax(bool state)
    {
        dynamic_max = state;
    }
};
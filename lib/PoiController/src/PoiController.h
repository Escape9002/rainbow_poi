#include <cstdint>

#include <LowPass.h>
#include <HSV.h>
#include <EffectEngine.h>

enum class POI_MODE {
    ACCELERATION,
    GYRO,
    CONSTANT,
    LOW_BATTERY
};

class PoiController
{
private:
    const uint32_t VALUE_MAX = 1000;
    uint32_t value_max_dyn = VALUE_MAX;
    bool dynamic_max = false;

    const uint32_t NORM_SCALE = 1000;

    uint32_t min, max;

    LowPassFilter<uint32_t> lPass;
    HSV hsv;

    POI_MODE current_mode = POI_MODE::ACCELERATION;

    EffectEngine* effectEngine;

    // TODO potential overroll here, since signed INT used
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

    uint32_t filter(const uint32_t value)
    {
        return lPass.filter(value);
    }

    HSV map_color(const uint32_t value)
    {

        uint8_t hue = static_cast<uint8_t>((static_cast<uint32_t>(hsv.hueMapper(min, max, value)) * 255) / 360);

        return HSV{
            hue,
            255,
            255};
    }

    HSV acceleration_ani(uint32_t value)
    {

        if (dynamic_max)
        {
            if (value > VALUE_MAX && value > value_max_dyn)
            {
                value_max_dyn = value;
            }
            else if (value_max_dyn > VALUE_MAX)
            {
                value_max_dyn -= 10;
            }
        }

        uint32_t normalized = normalize(value);
        uint32_t filtered = filter(normalized);

        return map_color(filtered);
    }

public:
    /**
     * @brief Construct a new Poi Controller object
     *
     * @param value_max maximum accleration
     * @param norm_scale scale on which to operate concerning float to fix-point
     * @param alpha lowPass alpha
     * @param min hueMin
     * @param max hueMax
     * @param dynamic_max enable dynamic maximum acceleration
     */
    PoiController(uint32_t value_max,
                  uint32_t norm_scale,
                  uint32_t alpha,
                  uint32_t min,
                  uint32_t max,
                  bool dynamic_max,
                EffectEngine* engine)
        : VALUE_MAX(value_max),
          NORM_SCALE(norm_scale),
          min(min),
          max(max),
          dynamic_max(dynamic_max),
          lPass(alpha),
          effectEngine(engine)

    {
        hsv = HSV{255, 255, 255};
    }

    HSV tick(int32_t value, uint32_t dt_ms)
    {

        switch (current_mode)
        {
        case POI_MODE::ACCELERATION:
            return acceleration_ani(value);

            break;

        case POI_MODE::LOW_BATTERY:
            return effectEngine->flash(HSV{0, 255,255}, 500);
            break;

        case POI_MODE::GYRO:
        case POI_MODE::CONSTANT:
        default:
            return effectEngine->rainbow(10);
        }
    }

    void setBatteryLevel(uint8_t batteryPercentage)
    {
        if (batteryPercentage < 10 && current_mode != POI_MODE::LOW_BATTERY)
        {
            // Save the mode so we can return to it if plugged in

            current_mode = POI_MODE::LOW_BATTERY;
        }
        else if (batteryPercentage >= 10 && current_mode == POI_MODE::LOW_BATTERY)
        {
            // this must be a sensor error, we can not charge the battery while the Board is powered
        }
    }

    void setAlpha(uint32_t alpha)
    {
        this->lPass.setAlpha(alpha);
    }

    uint32_t getAlpha()
    {
        return lPass.getAlpha();
    }

    void setColorRange(uint32_t min, uint32_t max)
    {
        this->min = min;
        this->max = max;
    }

    uint32_t getColorMin()
    {
        return this->min;
    }

    uint32_t getColorMax()
    {
        return this->max;
    }

    void setDynamicMax(bool state)
    {
        dynamic_max = state;
    }
};
#include <cstdint>

#include <LowPass.h>
#include <HSV.h>
#include <EffectEngine.h>

enum class POI_MODE
{
    ACCELERATION,
    GYRO,
    CONSTANT,
    LOW_BATTERY
};

class PoiController
{
private:
    // --------------------------------------------------------
    // HARDCODED VARIABLES
    // --------------------------------------------------------

    // maximum value (like acclereation) to expect
    // TODO: validate that the "const" actually does anything
    const uint32_t VALUE_MAX = 1000;
    uint32_t value_max_dyn = VALUE_MAX;
    bool dynamic_max = false;

    // our filters and stuff work with a scale of 0 to NORM_SCALE
    const uint32_t NORM_SCALE = 1000;

    uint32_t hueMin, hueMax;

    // --------------------------------------------------------
    // OBJECT VARS and PARAMS
    // --------------------------------------------------------

    LowPassFilter<uint32_t> lPass;
    HSV hsv;

    POI_MODE current_mode = POI_MODE::ACCELERATION;

    EffectEngine *effectEngine;

    // --------------------------------------------------------
    // HELPER FUNCTIONS
    // --------------------------------------------------------

    int32_t normalize(int32_t value);
    uint32_t filter(const uint32_t value);
    HSV map_color(const uint32_t value);
    HSV acceleration_ani(uint32_t value);

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
                  uint32_t hueMin,
                  uint32_t hueMax,
                  bool dynamic_max,
                  EffectEngine *engine)
        : VALUE_MAX(value_max),
          NORM_SCALE(norm_scale),
          hueMin(hueMin),
          hueMax(hueMax),
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
            return effectEngine->flash(HSV{0, 255, 255}, 500);
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

    void setAlpha(uint32_t alpha);
    uint32_t getAlpha();

    void setColorRange(uint32_t min, uint32_t max);
    uint32_t getHueMin();
    uint32_t getHueMax();

    void setDynamicMax(bool state);

    POI_MODE getMode();
    const char* getModeStr();
    void setMode(POI_MODE mode);
};
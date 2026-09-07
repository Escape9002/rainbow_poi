#include <cstdint>

#include <LowPass.h>
#include <HSV.h>
#include <EffectEngine.h>
#include <HAL.h>

enum class POI_MODE
{
    ACCELERATION,
    GYRO,
    CONSTANT,
    LOW_BATTERY,
    SLEEP
};

class PoiController
{
private:
    // --------------------------------------------------------
    // HARDCODED VARIABLES
    // --------------------------------------------------------

    // maximum value (like acclereation) to expect
    // TODO: validate that the "const" actually does anything
    const uint32_t ACCL_MAX = 1000;
    uint32_t accl_max_dyn = ACCL_MAX;
    bool dynamic_max = false;

    const uint32_t GYRO_MAX = 1000;
    uint32_t gyro_max_dyn = GYRO_MAX;

    // our filters and stuff work with a scale of 0 to NORM_SCALE
    const uint32_t NORM_SCALE = 1000;

    uint32_t hueMin, hueMax;

    int32_t lastAccl = 0;
    static const uint32_t JITTER_THRESHOLD = 200;
    uint32_t idle_time_ms = 0;
    static const uint32_t NO_MOTION_TIMEOUT_MS = 10 * 1000; // ms

    // --------------------------------------------------------
    // OBJECT VARS and PARAMS
    // --------------------------------------------------------

    LowPassFilter<uint32_t> lPass;
    HSV hsv;

    POI_MODE current_mode = POI_MODE::GYRO;

    EffectEngine *effectEngine;
    HAL *hal;

    // --------------------------------------------------------
    // HELPER FUNCTIONS
    // --------------------------------------------------------
    HSV animate(uint32_t max_variable, uint32_t &dynamic_max_variable, uint32_t value);
    int32_t normalize(int32_t value, int32_t value_max);
    uint32_t filter(const uint32_t value);
    HSV map_color(const uint32_t value);
    HSV acceleration_ani(uint32_t value);
    HSV gyro_ani(uint32_t value);

    bool no_movement(int32_t value);

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
    PoiController(uint32_t accl_max,
                  uint32_t gyro_max,
                  uint32_t norm_scale,
                  uint32_t alpha,
                  uint32_t hueMin,
                  uint32_t hueMax,
                  bool dynamic_max,
                  EffectEngine *engine,
                  HAL *hal)
        : ACCL_MAX(accl_max),
          NORM_SCALE(norm_scale),
          hueMin(hueMin),
          hueMax(hueMax),
          dynamic_max(dynamic_max),
          lPass(alpha),
          effectEngine(engine),
          hal(hal)

    {
        hsv = HSV{255, 255, 255};
    }

    HSV tick(int32_t value, uint32_t dt_ms)
    {
        // TODO differentiate between next color-state thingy and
        //  next Automat-State thingy
        ////////////////////////////////////////////////////
        /// HARDWARE STATE CHECK
        ////////////////////////////////////////////////////
        if (no_movement(value))
        {
            idle_time_ms += dt_ms;
        }
        else
        {
            idle_time_ms = 0;
        }

        if (idle_time_ms > NO_MOTION_TIMEOUT_MS)
        {
            this->current_mode = POI_MODE::SLEEP;
        }

        ////////////////////////////////////////////////////
        /// AUTOMATON
        ////////////////////////////////////////////////////
        switch (current_mode)
        {
        case POI_MODE::ACCELERATION:
            return acceleration_ani(value);

            break;

        case POI_MODE::LOW_BATTERY:
            return effectEngine->flash(HSV{0, 255, 255}, 500);
            break;

        case POI_MODE::SLEEP:
            this->hal->enterDeepSleep();
            return HSV{0, 0, 0};
            break;

        case POI_MODE::GYRO:
            return gyro_ani(value);
            break;

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
    const char *getModeStr();
    void setMode(POI_MODE mode);
};
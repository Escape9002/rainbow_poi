#pragma once
#include <cstdint>

struct HSV
{
    uint16_t hue; // 0 - 360
    uint8_t saturation;
    uint8_t brightness;

    /**
     * @brief Map "value" in the range of min to max
     *
     * @param min 0-360
     * @param max 0-360
     * @param value 0-1000
     * @return HSV
     */
    void hueMapper(uint16_t min, uint16_t max, uint32_t value)
    {
        // limit to the allowed maximum value
        if (value > 1000)
        {
            value = 1000;
        }

        // TODO should we catch the min,max outofrange errors?

        /**
         * Hue: [0 - 360]
         *
         * hue = ((max - min) * value) / 1000
         */

        // explizit wrap around, since hue is defined on a color-wheel
        hue = (((max - min) * value) / 1000) % 360;
    }

    /**
     * @brief Map "value" in the range of min to max
     *
     * @param min 0-360
     * @param max 0-360
     * @param value 0-1000
     * @return HSV
     */
    void saturationMapper(uint16_t min, uint16_t max, uint32_t value)
    {
        uint32_t exact_saturation = ((max - min) * value) / 1000;

        // ensure safe conversion to uint8_t
        if (exact_saturation > 255)
        {
            exact_saturation = 255;
        }

        saturation = (uint8_t)exact_saturation;
    }

    /**
     * @brief Map "value" in the range of min to max
     *
     * @param min 0-360
     * @param max 0-360
     * @param value 0-1000
     * @return HSV
     */
    void brightnessMapper(uint16_t min, uint16_t max, uint32_t value)
    {
        uint32_t exact_brightness = ((max - min) * value) / 1000;

        // ensure safe conversion to uint8_t
        if (exact_brightness > 255)
        {
            exact_brightness = 255;
        }

        brightness = (uint8_t)exact_brightness;
    }
};
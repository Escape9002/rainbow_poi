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
    public:void hueMapper(uint16_t min, uint16_t max, uint32_t value)
    {
        // limit to the allowed maximum value
        if (value > 1000)
        {
            value = 1000;
        }

        int32_t signed_min = (int32_t) min;
        int32_t signed_max = (int32_t) max;
        int32_t signed_value = (int32_t) value;

        
        int32_t mapped = signed_min - ((signed_min-signed_max)*signed_value) / 1000;
        // explizit wrap around, since hue is defined on a color-wheel
        mapped %= 360;

        // ensure mapped is positive value
        if(mapped < 0 ){
            mapped += 360;
        }

        hue = (uint16_t) mapped;
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
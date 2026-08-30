#pragma once
#include <cstdint>

struct HSV {
    uint16_t hue; // 0 - 360
    uint8_t saturation; 
    uint8_t brightness;
};

class HSVMapper {
    private:
    uint32_t min;
    uint32_t max;


    public:

    /**
     * @brief Map "value" in the range of min to max
     * 
     * @param min 0-360
     * @param max 0-360
     * @param value 0-1000
     * @return HSV 
     */
    HSV colorMapper(uint16_t min, uint16_t max, uint32_t value);
    HSV saturationMapper(uint16_t min, uint16_t max, uint32_t value);
    HSV brightnessMapper(uint16_t min, uint16_t max, uint32_t value);
};

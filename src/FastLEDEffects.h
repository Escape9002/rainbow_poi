#pragma once
#include <FastLED.h>
#include <EffectEngine.h>

class FastLEDEffects : public EffectEngine
{
private:
    uint16_t current_hue = 0;

public:
    HSV flash(HSV baseColor, uint32_t interval_ms) override
    {
        uint32_t now = millis();
        if ((now % interval_ms) < (interval_ms / 2)) {
            return baseColor; // ON
        }
        return HSV{0, 0, 0};  // OFF
    }

    HSV breathe(HSV baseColor, uint8_t bpm) override
    {
        // Use FastLED's highly optimized 8-bit sine wave generator
        uint8_t brightness = beatsin8(bpm, 0, 255);
        return HSV{baseColor.hue, baseColor.saturation, brightness};
    }

    HSV rainbow(uint32_t circles) override
    {
        // Use FastLED's beat generator for smooth rainbow cycling
        uint8_t hue = beat8(circles); // Cycle a full 255 rainbow 30 times a minute
        
        // Map FastLED's 0-255 hue back to our abstract 0-360 degree hue
        uint16_t mapped_hue = (hue * 360) / 255;
        return HSV{mapped_hue, 255, 255};
    }
};
#pragma once
#include "HSV.h"
#include <cstdint>

class EffectEngine
{
public:
    // Virtual destructor is required for interfaces
    virtual ~EffectEngine() = default;

    // The effects your controller can ask for
    virtual HSV flash(HSV baseColor, uint32_t interval_ms) = 0;
    virtual HSV breathe(HSV baseColor, uint8_t bpm) = 0;
    virtual HSV rainbow(uint32_t dt_ms) = 0;
};
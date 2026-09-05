#include <PoiController.h>

int32_t PoiController::normalize(int32_t value)
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

uint32_t PoiController::filter(const uint32_t value)
{
    return lPass.filter(value);
}

HSV PoiController::map_color(const uint32_t value)
{

    uint8_t hue = static_cast<uint8_t>((static_cast<uint32_t>(hsv.hueMapper(hueMin, hueMax, value)) * 255) / 360);

    return HSV{
        hue,
        255,
        255};
}

HSV PoiController::acceleration_ani(uint32_t value)
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

void PoiController::setAlpha(uint32_t alpha)
{
    this->lPass.setAlpha(alpha);
}

uint32_t PoiController::getAlpha()
{
    return lPass.getAlpha();
}

void PoiController::setColorRange(uint32_t hueMin, uint32_t hueMax)
{
    this->hueMin = hueMin%360;
    this->hueMax = hueMax%360;
}

uint32_t PoiController::getHueMin()
{
    return this->hueMin;
}

uint32_t PoiController::getHueMax()
{
    return this->hueMax;
}

void PoiController::setDynamicMax(bool state)
{
    dynamic_max = state;
}
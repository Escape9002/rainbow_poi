#include <PoiController.h>
#include <string>

int32_t PoiController::normalize(int32_t value, int32_t value_max_dyn)
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

HSV PoiController::animate(uint32_t max_variable, uint32_t &dynamic_max_variable, int32_t value)
{
    if (dynamic_max)
    {
        if (value > max_variable && value > dynamic_max_variable)
        {
            dynamic_max_variable = value;
        }
        else
        {
            dynamic_max_variable -= 10;
        }
    }

    uint32_t normalized = normalize(value, dynamic_max_variable);
    uint32_t filtered = filter(normalized);

    return map_color(filtered);
}

HSV PoiController::acceleration_ani(uint32_t value)
{
    return animate(ACCL_MAX, accl_max_dyn, value);
}

HSV PoiController::gyro_ani(uint32_t value)
{
    return animate(GYRO_MAX, gyro_max_dyn, value);
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
    this->hueMin = hueMin % 360;
    this->hueMax = hueMax % 360;
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

POI_MODE PoiController::getMode()
{
    return this->current_mode;
}

const char *PoiController::getModeStr()
{
    switch (this->getMode())
    {
    case POI_MODE::ACCELERATION:
        return "ACCL";

    case POI_MODE::CONSTANT:
        return "CONS";

    case POI_MODE::GYRO:
        return "GYRO";

    case POI_MODE::LOW_BATTERY:
        return "LWBT";

    case POI_MODE::SLEEP:
        return "SLEEP";
        
    default:
        return "ERRO";
    }
}

void PoiController::setMode(POI_MODE mode)
{
    this->current_mode = mode;
}

bool PoiController::no_movement(int32_t accl)
{
    int32_t diff = accl - lastAccl;

    lastAccl = accl;

    return std::abs(diff) < JITTER_THRESHOLD;
}
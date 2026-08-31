#include <cstdint>
class LowPass
{
    
private:
    // allow only from 0 to 1000
    uint32_t alpha;
    static constexpr uint32_t FILTER_SCALE = 1000;

    int32_t previous = 0;
    bool initialized = false;

public:
    LowPass(uint32_t alpha)
    {
        if (alpha > FILTER_SCALE)
        {
            alpha = FILTER_SCALE;
        }

        this->alpha = alpha;
    }

    int32_t filter(int32_t input)
    {
        if (!initialized)
        {
            previous = input;
            initialized = true;
            return previous;
        }

        int32_t error = input - previous;
        int32_t correction = ((int32_t)alpha * error) / (int32_t)FILTER_SCALE;

        previous += correction;

        return previous;
    }
};
#pragma once
#include <cstdint>

constexpr uint32_t FILTER_SCALE = 1000;

template <typename T>
struct FilterCalcType
{
    using type = std::conditional_t<
        sizeof(T) <= 1, // int8 is 1 byte
        int16_t,        // T =< 1 byte -> computation at int16
        std::conditional_t<
            sizeof(T) <= 2, // int16 is 2 byte
            int32_t,        // T =< 2 byte -> computation at int32
            int64_t         // T > 2 byte -> computation at int64
            >>;
};

template <typename T>
class LowPassFilter
{
    static_assert(
        std::is_integral_v<T>,
        "LowPassFilter requires an integral type");

private:
    T previous = 0;
    uint32_t alpha;

    bool initialized = false;

    using CalcType =
        typename FilterCalcType<T>::type;

public:
    LowPassFilter(const uint32_t alpha)
    {
        if (alpha > FILTER_SCALE)
        {
            alpha = FILTER_SCALE;
        }

        this->alpha = alpha;
    }

    T filter(const T input)
    {
        if (!initialized)
        {
            previous = input;
            initialized = true;
            return previous;
        }

        CalcType error = static_cast<CalcType>(input) - static_cast<CalcType>(previous);
        CalcType correction = (alpha * error) / FILTER_SCALE;

        previous = static_cast<T>(static_cast<CalcType>(previous) + correction);

        return previous;
    }

    void setAlpha(uint32_t alpha)
    {
        this->alpha = alpha;
    }

    uint32_t getAlpha()
    {
        return this->alpha;
    }
};
#pragma once
#include <cstdint>
#include <type_traits>
#include <limits>

constexpr uint32_t FILTER_SCALE = 1000;

template <typename T>
struct FilterCalcType
{
    using type = typename std::conditional<
        sizeof(T) <= 1,
        int16_t,
        typename std::conditional<
            sizeof(T) <= 2,
            int32_t,
            int64_t
        >::type
    >::type;
};

template <typename T>
class LowPassFilter
{
    static_assert(
        std::is_integral<T>::value,
        "LowPassFilter requires an integral type");

private:
    T previous = 0;
    uint32_t alpha;

    bool initialized = false;

    using CalcType =
        typename FilterCalcType<T>::type;

public:
    LowPassFilter(uint32_t alpha)
    {
        setAlpha(alpha);
    }

    // T filter(T input)
    // {
    //     if (!initialized)
    //     {
    //         previous = input;
    //         initialized = true;
    //         return previous;
    //     }

    //     CalcType error = static_cast<CalcType>(input) - static_cast<CalcType>(previous);
    //     CalcType correction = (alpha * error) / FILTER_SCALE;

    //     previous = static_cast<T>(static_cast<CalcType>(previous) + correction);

    //     return previous;
    // }

    T filter(T input)
    {
        if (!initialized)
        {
            previous = input;
            initialized = true;
            return input;
        }

        CalcType error =
            static_cast<CalcType>(input) -
            static_cast<CalcType>(previous);

        CalcType correction =
            (error * alpha) / FILTER_SCALE;

        previous =
            static_cast<T>(
                static_cast<CalcType>(previous) +
                correction);

        return previous;
    }

    void setAlpha(uint32_t alpha)
    {
        if (alpha > FILTER_SCALE)
        {
            alpha = FILTER_SCALE;
        }
        this->alpha = alpha;
    }

    uint32_t getAlpha()
    {

        return this->alpha;
    }
};
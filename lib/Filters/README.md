# Filters
Input: int32
output: int32
intermediate: int32

1. programm for in32
2. switch to template for input/output
3. switch to template/guess for intermediate

```cpp
template <typename T>
class LowPassFilter
{
private:
    T previous;
    uint16_t alpha;
    bool initialized;

public:

    LowPassFilter(uint16_t alpha)
        : previous(0),
          alpha(alpha),
          initialized(false)
    {
    }

    T filter(T input);
};
```
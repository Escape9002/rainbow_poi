#pragma once
#include <cstdint>

class HAL{
 public:
 virtual ~HAL() = default;
 
 virtual void enterDeepSleep() = 0;
};
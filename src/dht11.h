#pragma once
#include "pico/stdlib.h"

class DHT11
{
public:
    explicit DHT11(uint gpio);
    bool read(float *temperature, float *humidity);

private:
    uint gpio_;
};

#pragma once
#include <Arduino.h>

class I2CHandler {
public:
    static void begin(uint8_t addr);
};
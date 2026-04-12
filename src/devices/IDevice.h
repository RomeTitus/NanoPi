#pragma once
#include <Arduino.h>

class IDevice {
public:
    virtual String handleText(String input) = 0;
    virtual void handleBinary(uint8_t* data, uint8_t len, uint8_t* response, uint8_t& responseLen) = 0;
};
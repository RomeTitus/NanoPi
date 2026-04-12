#pragma once
#include <Arduino.h>

class IDevice {
public:
    virtual ~IDevice() {}

    virtual void handleBinary(uint8_t* data,
                              uint8_t len,
                              uint8_t* response,
                              uint8_t& responseLen) = 0;

    virtual bool validate(uint8_t* data,
                          uint8_t len,
                          String& error) = 0;
};
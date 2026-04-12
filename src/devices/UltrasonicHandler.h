#pragma once

#include "IDevice.h"

class UltrasonicHandler : public IDevice {
public:
    UltrasonicHandler();

    void handleBinary(uint8_t* data,
                      uint8_t len,
                      uint8_t* response,
                      uint8_t& responseLen) override;

    bool validate(uint8_t* data,
                  uint8_t len,
                  String& error) override;

    long readDistance(uint8_t trigPin, uint8_t echoPin);
};
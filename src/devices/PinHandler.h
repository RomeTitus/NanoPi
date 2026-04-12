#pragma once
#include "IDevice.h"
class PinHandler : public IDevice {
public:
    PinHandler();

    void handleBinary(uint8_t* data,
                      uint8_t len,
                      uint8_t* response,
                      uint8_t& responseLen) override;

    bool validate(uint8_t* data,
                  uint8_t len,
                  String& error) override;

private:

    void handleDigitalWrite(uint8_t pin, uint8_t state);
    int handleDigitalRead(uint8_t pin);
};
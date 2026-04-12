#pragma once
#include "IDevice.h"
class PinHandler : public IDevice {
public:
    PinHandler();

    String handleText(String input) override;

    void handleBinary(uint8_t* data,
                      uint8_t len,
                      uint8_t* response,
                      uint8_t& responseLen) override;

    CommandSchema getSchema() override;

    bool validate(uint8_t* data,
                  uint8_t len,
                  String& error) override;

private:
    CommandSchema schema;

    void handleDigitalWrite(uint8_t pin, uint8_t state);
    int handleDigitalRead(uint8_t pin);
};
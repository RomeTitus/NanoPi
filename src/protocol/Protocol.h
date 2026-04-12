#pragma once
#include "../devices/DeviceRegistry.h"

class Protocol {
private:
    DeviceRegistry* registry;

public:
    Protocol(DeviceRegistry* reg);

    String processInput(String input);
    void processBinary(uint8_t* data, uint8_t len, uint8_t* response, uint8_t& responseLen);

private:
    bool isBinary(uint8_t* data, uint8_t len);
    String processText(String input);
    void translateToBinary(String input, uint8_t* out, uint8_t& outLen);
};
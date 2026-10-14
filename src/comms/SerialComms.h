#pragma once
#include <Arduino.h>
#include "../protocol/Protocol.h"

class SerialComms {
private:
    Protocol* protocol;
    char input[96] = {};
    uint8_t used = 0;
    bool overflow = false;

public:
    SerialComms(Protocol* proto);

    void begin(unsigned long baud = 9600);
    void loop();
};

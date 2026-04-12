#pragma once
#include <Arduino.h>
#include "../protocol/Protocol.h"

class SerialComms {
private:
    Protocol* protocol;

public:
    SerialComms(Protocol* proto);

    void begin(unsigned long baud = 9600);
    void loop();
};
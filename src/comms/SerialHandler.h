#pragma once
#include <Arduino.h>

class SerialHandler {
public:
    static void begin(long baud);
    static void loop();
};
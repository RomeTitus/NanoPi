#pragma once
#include <Arduino.h>

class PinHandler {
public:
    static void handle(uint8_t* req, uint8_t* res);
    static void handleText(char* cmd, char* out);
};
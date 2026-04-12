#pragma once
#include <Arduino.h>

struct Ultrasonic {
    uint8_t trig;
    uint8_t echo;
    bool active;
};

class UltrasonicHandler {
public:
    static void handle(uint8_t* req, uint8_t* res);
    static void handleText(char* cmd, char* out);

private:
    static uint16_t read(Ultrasonic &d);
};
#pragma once
#include <Arduino.h>

#define MAX_TRANSLATIONS 10

class TextTranslator {
private:
    String keys[MAX_TRANSLATIONS];
    uint8_t commands[MAX_TRANSLATIONS];
    uint8_t count = 0;

public:
    void registerTranslation(String key, uint8_t cmd);
    bool translate(String input, uint8_t* out, uint8_t& outLen);
};
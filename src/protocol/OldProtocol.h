#pragma once
#include <Arduino.h>

#define BUFFER_SIZE 64

class Protocol {
public:
    static void processInput(uint8_t* data, int len, char* response);

    static void processBinary(uint8_t* data, int len, uint8_t* response);
    static void processText(char* input, char* output);

private:
    static void processSingle(char* cmd, char* out);
};
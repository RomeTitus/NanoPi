#pragma once
#include <Arduino.h>


#define MAX_ARGS 5

enum ArgType {
    ARG_UINT8,
    ARG_BOOL,
    ARG_ENUM
};

struct ArgRule {
    ArgType type;
    uint8_t minValue;
    uint8_t maxValue;
};

struct CommandSchema {
    uint8_t cmd;
    const char* name;
    uint8_t argCount;
    ArgRule args[MAX_ARGS];
};

class IDevice {
public:
    virtual ~IDevice() {}
    virtual String handleText(String input) = 0;

    virtual void handleBinary(uint8_t* data,
                              uint8_t len,
                              uint8_t* response,
                              uint8_t& responseLen) = 0;

    virtual CommandSchema getSchema() = 0;

    virtual bool validate(uint8_t* data,
                          uint8_t len,
                          String& error) = 0;
};
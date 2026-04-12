#include "PinHandler.h"

PinHandler::PinHandler() {

    schema = {
        0x01,
        "Pin",
        3,
        {
            {ARG_UINT8, 0, 13},  // pin
            {ARG_ENUM, 0, 1},    // mode (0=input, 1=output)
            {ARG_BOOL, 0, 1}     // state
        }
    };
}

CommandSchema PinHandler::getSchema() {
    return schema;
}

bool PinHandler::validate(uint8_t* data, uint8_t len, String& error) {

    if (len - 1 < schema.argCount) {
        error = "Pin: missing args";
        return false;
    }

    uint8_t pin = data[1];
    uint8_t mode = data[2];
    uint8_t state = data[3];

    // pin range check
    if (pin > 13) {
        error = "Pin: invalid pin";
        return false;
    }

    // mode check
    if (mode > 1) {
        error = "Pin: invalid mode";
        return false;
    }

    // state check
    if (state > 1) {
        error = "Pin: invalid state";
        return false;
    }

    return true;
}

String PinHandler::handleText(String input) {
    // Optional: keep simple or route through binary pipeline
    return "OK";
}

void PinHandler::handleBinary(uint8_t* data,
                              uint8_t len,
                              uint8_t* response,
                              uint8_t& responseLen) {

    uint8_t pin = data[1];
    uint8_t mode = data[2];
    uint8_t state = data[3];

    if (mode == 1) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, state);
    } else {
        pinMode(pin, INPUT);
        state = digitalRead(pin);
    }

    response[0] = 0xAA;
    response[1] = (state >> 8) & 0xFF;
    response[2] = state & 0xFF;
    responseLen = 3;
}
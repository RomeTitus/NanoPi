#include "PinHandler.h"

String PinHandler::handleText(String input) {
    // Example: "Pin,2D,On"
    return "Ok";
}

void PinHandler::handleBinary(uint8_t* data, uint8_t len, uint8_t* response, uint8_t& responseLen) {
    // Example: [cmd, pin, state]
    response[0] = 0xAA;
    response[1] = 0x01;
    responseLen = 2;
}
#include "UltrasonicHandler.h"

String UltrasonicHandler::handleText(String input) {
    return String(234); // dummy
}

void UltrasonicHandler::handleBinary(uint8_t* data, uint8_t len, uint8_t* response, uint8_t& responseLen) {
    int distance = 234;

    response[0] = 0xAA;
    response[1] = (distance >> 8) & 0xFF; // high
    response[2] = distance & 0xFF;        // low
    responseLen = 3;
}
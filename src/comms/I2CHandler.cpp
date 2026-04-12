#include "I2CHandler.h"
#include <Wire.h>
#include "../protocol/Protocol.h"

static uint8_t buffer[64];
static char response[64];

void receiveEvent(int count) {

    int len = 0;

    while (Wire.available() && len < 64) {
        buffer[len++] = Wire.read();
    }

    Protocol::processInput(buffer, len, response);
}

void requestEvent() {
    if (response[0] == 0xBB) {
        Wire.write((uint8_t*)response, 5);
    } else {
        Wire.write(response);
    }
}

void I2CHandler::begin(uint8_t addr) {
    Wire.begin(addr);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}
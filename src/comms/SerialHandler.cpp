#include "SerialHandler.h"
#include "../protocol/Protocol.h"

static uint8_t buffer[64];
static char response[64];
static int index = 0;

void SerialHandler::begin(long baud) {
    Serial.begin(baud);
}

void SerialHandler::loop() {

    while (Serial.available()) {

        uint8_t b = Serial.read();

        if (b == '\n') {
            buffer[index] = '\0';
            Protocol::processInput(buffer, index, response);
            Serial.println(response);
            index = 0;
        } else {
            buffer[index++] = b;
            if (index >= 64) index = 0;
        }
    }
}
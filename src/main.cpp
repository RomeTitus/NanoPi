#include <Arduino.h>
#include "comms/I2CHandler.h"
#include "comms/SerialHandler.h"

void setup() {

    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);

    I2CHandler::begin(0x04);
    SerialHandler::begin(9600);
}

void loop() {
    SerialHandler::loop();
}
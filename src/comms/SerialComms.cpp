#include "SerialComms.h"

void SerialComms::loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        String result = protocol->processInput(input);
        Serial.println(result);
    }
}
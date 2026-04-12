#include "SerialComms.h"

SerialComms::SerialComms(Protocol* proto) {
    protocol = proto;
}

void SerialComms::begin(unsigned long baud) {
    Serial.begin(baud);
}

void SerialComms::loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        
        String result = protocol->processInput(input);
        Serial.println(result);
    }
}
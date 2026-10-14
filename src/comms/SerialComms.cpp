#include "SerialComms.h"

SerialComms::SerialComms(Protocol* proto) {
    protocol = proto;
}

void SerialComms::begin(unsigned long baud) {
    Serial.begin(baud);
}

void SerialComms::loop() {
    // An unfinished USB command must not block RS485 service for a second.
    while (Serial.available()) {
        char c = char(Serial.read());
        if (c == '\r') continue;
        if (c == '\n') {
            input[used] = '\0';
            if (overflow) Serial.println("ERR");
            else if (used) Serial.println(protocol->processInput(String(input)));
            used = 0;
            overflow = false;
            return;
        }
        if (used < sizeof(input) - 1) input[used++] = c;
        else overflow = true;
    }
}

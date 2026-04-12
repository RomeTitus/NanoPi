#include "PinHandler.h"
#include <string.h>

void PinHandler::handle(uint8_t* req, uint8_t* res) {

    uint8_t cmdId = req[1];
    uint8_t action = req[3];
    uint8_t pinType = req[4];
    uint8_t pin = req[5];
    uint8_t value = req[6];

    uint16_t result = 0;
    uint8_t status = 0;

    if (action == 0) {
        result = (pinType == 1) ? analogRead(pin) : digitalRead(pin);
    } else {
        pinMode(pin, OUTPUT);
        if (pinType == 1)
            analogWrite(pin, value ? 255 : 0);
        else
            digitalWrite(pin, value);
    }

    res[0] = 0xBB;
    res[1] = cmdId;
    res[2] = status;
    res[3] = result >> 8;
    res[4] = result & 0xFF;
}

void PinHandler::handleText(char* cmd, char* out) {

    char* saveptrHandleText;
    char* pinStr = strtok_r(cmd, ",", &saveptrHandleText);
    char* type = strtok_r(NULL, ",", &saveptrHandleText);
    char* value = strtok_r(NULL, ",", &saveptrHandleText);

    int pin = atoi(pinStr + 1);

    if (strcmp(type, "read") == 0) {
        int val = (pinStr[0] == 'A') ? analogRead(pin) : digitalRead(pin);
        itoa(val, out, 10);
    } else {
        int output = (value && strcmp(value, "On") == 0) ? HIGH : LOW;

        pinMode(pin, OUTPUT);

        if (pinStr[0] == 'A')
            analogWrite(pin, output ? 255 : 0);
        else
            digitalWrite(pin, output);

        strcpy(out, "OK");
    }
}
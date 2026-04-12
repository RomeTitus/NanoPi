#include "PinHandler.h"

PinHandler::PinHandler() {
}

bool PinHandler::validate(uint8_t* data, uint8_t len, String& error) {
    if (len - 1 < 3) {
        error = "Pin: missing args";
        return false;
    }

    //0 0 2 1 DeviceId, Write or Read, Digital or Analog, pin value
    uint8_t mode = data[1]; //Write or Read
    uint8_t pinType = data[2];
    uint8_t pin = data[3];
    uint8_t state = 0;
    if(mode == 0) { //write
        state = data[4];
    }

    // pin range check
    if (pin > 13 || (pinType == 1 && (pin == 5 || pin == 6 || pin > 7))) {
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

void PinHandler::handleBinary(uint8_t* data,
                              uint8_t len,
                              uint8_t* response,
                              uint8_t& responseLen) {

    uint8_t mode = data[1]; //Write or Read
    uint8_t pinType = data[2];
    uint8_t pin = data[3];
    uint8_t state = 0;
        if(mode == 0) { //write
        state = data[4];
    }
    
    if (mode == 0) {
        pinMode(pin, OUTPUT);
        if(pinType == 0) { //digital  
            digitalWrite(pin, state);
        } else {
            // For simplicity, we treat analog write as digital for now (0 or 255), if we go below 255, it's treated as pwm
            analogWrite(pin, state == 1 ? 255 : 0);
        }
    } else {
        pinMode(pin, INPUT);
        if(pinType == 0) { //digital  
            state = digitalRead(pin);
        } else {
            state = analogRead(pin);
        }
    }

    response[0] = 0xAA;
    response[1] = (state >> 8) & 0xFF;
    response[2] = state & 0xFF;
    responseLen = 3;
}
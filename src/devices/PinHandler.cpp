#include "PinHandler.h"

PinHandler::PinHandler() {
}

bool PinHandler::validate(uint8_t* data, uint8_t len, String& error) {
    if (len < 3) {
        error = "Pin: missing args";
        return false;
    }

    // DeviceId, Write or Read, pin number (0-13 digital, 14-21 for A0-A7), [state for writes]
    uint8_t mode = data[1];  // Write or Read
    uint8_t pin = data[2];   // Pin number
    
    if(mode == 0) {  // write - need state
        if (len < 4) {
            error = "Pin: missing state";
            return false;
        }
    }

    // pin range check (0-13 digital, 14-17 and 20-21 for A0-A3 and A6-A7)
    // A4 (pin 18) and A5 (pin 19) are reserved for I2C (SDA/SCL)
    if (pin > 21 || pin == 18 || pin == 19) {
        error = "Pin: invalid pin";
        return false;
    }

    // mode check
    if (mode > 1) {
        error = "Pin: invalid mode";
        return false;
    }

    return true;
}

void PinHandler::handleBinary(uint8_t* data,
                              uint8_t len,
                              uint8_t* response,
                              uint8_t& responseLen) {

    uint8_t mode = data[1];  // Write or Read
    uint8_t pin = data[2];   // Pin number (0-13 digital, 14-21 for A0-A7)
    uint8_t state = 0;
    
    if(mode == 0) {  // write
        state = data[3];
    }
    
    bool isAnalog = pin >= 14;  // Pins 14-21 are analog (A0-A7)
    if (mode == 0) {  // Write
        pinMode(pin, OUTPUT);
        if(isAnalog) {  // analog - use analogWrite for PWM (0-255)
            analogWrite(pin, !state); //Need to reverse the sate for the relay board, which is active LOW
        } else {  // digital
            digitalWrite(pin, !state);
        }
    } else {  // Read
        pinMode(pin, INPUT);
        if(isAnalog) {  // analog
            state = analogRead(pin);
        } else {  // digital
            state = digitalRead(pin);
        }
    }

    response[0] = 0xAA;
    response[1] = (state >> 8) & 0xFF;
    response[2] = state & 0xFF;
    responseLen = 3;
}
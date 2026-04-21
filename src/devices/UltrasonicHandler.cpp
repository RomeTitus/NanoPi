#include "UltrasonicHandler.h"

UltrasonicHandler::UltrasonicHandler() {
}

bool UltrasonicHandler::validate(uint8_t* data, uint8_t len, String& error) {
    if (len - 1 < 2) { 
        error = "US: missing args";
        return false;
    }

    uint8_t trig = data[1];
    uint8_t echo = data[2];

    // A4 (pin 18) and A5 (pin 19) are reserved for I2C (SDA/SCL)
    if (trig > 21 || trig == 18 || trig == 19) {
        error = "Trig: invalid pin";
        return false;
    }

    // A4 (pin 18) and A5 (pin 19) are reserved for I2C (SDA/SCL)
    if (echo > 21 || echo == 18 || echo == 19) {
        error = "Echo: invalid pin";
        return false;
    }

    return true;
}

void UltrasonicHandler::handleBinary(uint8_t* data,
                                     uint8_t len,
                                     uint8_t* response,
                                     uint8_t& responseLen) {

    uint8_t trig = data[1];
    uint8_t echo = data[2];
    
    long distance = readDistance(trig, echo);

    response[0] = 0xAA;
    response[1] = (distance >> 8) & 0xFF;
    response[2] = distance & 0xFF;
    responseLen = 3;
}

long UltrasonicHandler::readDistance(uint8_t trigPin, uint8_t echoPin) {

    // Convert analog pins to digital equivalents (14-21 are A0-A7)
    uint8_t actualTrigPin = trigPin;
    uint8_t actualEchoPin = echoPin;
    
    if (trigPin >= 14) {
        actualTrigPin = trigPin;  // Arduino accepts 14-21 directly for analog pins
    }
    
    if (echoPin >= 14) {
        actualEchoPin = echoPin;
    }

    pinMode(actualTrigPin, OUTPUT);
    pinMode(actualEchoPin, INPUT);

    digitalWrite(actualTrigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(actualTrigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(actualTrigPin, LOW);

    long duration = pulseIn(actualEchoPin, HIGH);
    long distance = duration * 0.034 / 2;

    return distance;
}
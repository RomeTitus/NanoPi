#include "UltrasonicHandler.h"

UltrasonicHandler::UltrasonicHandler() {
}

bool UltrasonicHandler::validate(uint8_t* data, uint8_t len, String& error) {
    if (len - 1 < 3) { 
        error = "US: missing args";
        return false;
    }

    uint8_t trig = data[1];
    uint8_t trigType = data[2];

    uint8_t echo = data[3];
    uint8_t echoType = data[4];



    if (trig > 13 || (trigType == 1 && (trig == 5 || trig == 6 || trig > 7))) {
        error = "Trig: invalid pin";
        return false;
    }

    if (echo > 13 || (echoType == 1 && (echo == 5 || echo == 6 || echo > 7))) {
        error = "Echo: invalid pin";
        return false;
    }

    if (trig == echo) {
        error = "US: trig == echo invalid";
        return false;
    }

    return true;
}

void UltrasonicHandler::handleBinary(uint8_t* data,
                                     uint8_t len,
                                     uint8_t* response,
                                     uint8_t& responseLen) {

    uint8_t trig = data[1];
    uint8_t trigType = data[2];

    uint8_t echo = data[3];
    uint8_t echoType = data[4];
    
    //Add support or analog pins later (e.g., A0-A7)
    long distance = readDistance(trig, echo);

    response[0] = 0xAA;
    response[1] = (distance >> 8) & 0xFF;
    response[2] = distance & 0xFF;
    responseLen = 3;
}

long UltrasonicHandler::readDistance(uint8_t trigPin, uint8_t echoPin) {

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    //And if we have analog?
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    long distance = duration * 0.034 / 2;

    return distance;
}
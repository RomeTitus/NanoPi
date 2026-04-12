#include "UltrasonicHandler.h"

UltrasonicHandler::UltrasonicHandler() {

    schema = {
        0x02,
        "US",
        2,
        {
            {ARG_UINT8, 0, 13}, // trig
            {ARG_UINT8, 0, 13}  // echo
        }
    };
}

CommandSchema UltrasonicHandler::getSchema() {
    return schema;
}

bool UltrasonicHandler::validate(uint8_t* data, uint8_t len, String& error) {

    if (len - 1 < schema.argCount) {
        error = "US: missing args";
        return false;
    }

    uint8_t trig = data[1];
    uint8_t echo = data[2];

    if (trig > 13 || echo > 13) {
        error = "US: invalid pin";
        return false;
    }

    if (trig == echo) {
        error = "US: trig == echo invalid";
        return false;
    }

    return true;
}

String UltrasonicHandler::handleText(String input) {
    return String(234); // placeholder
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

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    long distance = duration * 0.034 / 2;

    return distance;
}
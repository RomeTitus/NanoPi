#include "UltrasonicHandler.h"
#include "../protocol/StatusCodes.h"

static const unsigned long ULTRASONIC_PULSE_TIMEOUT_US = 30000UL;
static const unsigned long ULTRASONIC_MIN_GAP_US = 60000UL;
static const uint8_t ULTRASONIC_RETRY_COUNT = 2;
static unsigned long s_lastPingUs = 0;

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
                                     uint8_t& responseLen,
                                     bool isDebugEnabled) {
    

    uint8_t trig = data[1];
    uint8_t echo = data[2];
    
    long distance = readDistance(trig, echo);

    if(isDebugEnabled) {
        Serial.print("UltrasonicHandler trig=");
        Serial.print(trig);
        Serial.print(" echo=");
        Serial.println(echo);
        Serial.print("UltrasonicHandler distance=");
        Serial.println(distance);
    }

    if (distance < 0) {
        response[0] = 0xAA;
        response[1] = StatusCodes::ERR_ULTRASONIC_TIMEOUT;
        responseLen = 2;
        return;
    }

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

    unsigned long nowUs = micros();
    unsigned long elapsedUs = nowUs - s_lastPingUs;
    if (elapsedUs < ULTRASONIC_MIN_GAP_US) {
        delayMicroseconds(ULTRASONIC_MIN_GAP_US - elapsedUs);
    }
    s_lastPingUs = micros();

    pinMode(actualTrigPin, OUTPUT);
    // INPUT_PULLUP keeps the echo line defined when disconnected,
    // preventing floating-pin noise from producing spurious readings.
    pinMode(actualEchoPin, INPUT_PULLUP);

    digitalWrite(actualTrigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(actualTrigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(actualTrigPin, LOW);

    for (uint8_t attempt = 0; attempt < ULTRASONIC_RETRY_COUNT; attempt++) {
        long duration = pulseIn(actualEchoPin, HIGH, ULTRASONIC_PULSE_TIMEOUT_US);

        if (duration != 0) {
            return duration * 0.034 / 2;
        }

        // If the echo was missed, allow a little more time before trying again.
        delayMicroseconds(5000);
    }

    return -1;
}
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "../protocol/Protocol.h"

#define SLAVE_ADDR 0x04

class I2CComms {
private:
    Protocol* protocol;

    // Response queue for batched messages
    static const uint8_t QUEUE_SIZE = 8;
    static const uint8_t RESPONSE_SIZE = 5;
    static const uint8_t INPUT_SIZE = QUEUE_SIZE * RESPONSE_SIZE; //8 x 5 byte messages max (we can increase this if needed)
    static uint8_t inputBuffer[INPUT_SIZE]; 
    static uint8_t responseQueue[QUEUE_SIZE][RESPONSE_SIZE];
    static uint8_t queueCount;

    static void onReceive(int len);
    static void onRequest();
    static void processPacket(uint8_t data[], uint8_t from, uint8_t to);

public:
    I2CComms(Protocol* proto);

    void begin();
};
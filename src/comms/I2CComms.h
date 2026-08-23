#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "../protocol/Protocol.h"

#define SLAVE_ADDR 0x04

class I2CComms {
private:
    Protocol* protocol;

    // I2C transport framing
    static const uint8_t MAX_I2C_FRAME = 32;
    static const uint8_t RESPONSE_SIZE = 4; // [status][value_hi][value_mid][value_lo]
    static const uint8_t QUEUE_SIZE = 7;    // 1 count + (7 * 4) + 1 checksum = 30 bytes
    static const uint8_t INPUT_SIZE = MAX_I2C_FRAME;
    static uint8_t inputBuffer[INPUT_SIZE]; 
    static uint8_t pendingBuffer[INPUT_SIZE];
    static uint8_t responseQueue[QUEUE_SIZE][RESPONSE_SIZE];
    static volatile uint8_t queueCount;
    static volatile uint8_t requestFlags;
    static volatile uint8_t pendingLen;
    static volatile uint8_t pendingFlags;
    static volatile bool requestPending;
    static volatile bool responseReady;

    static uint8_t calculateChecksum(const uint8_t* data, uint8_t len);
    static bool isDebugEnabled();
    static void packResponseRecord(const uint8_t* response,
                                   uint8_t responseLen,
                                   uint8_t outRecord[RESPONSE_SIZE]);

    static void onReceive(int len);
    static void onRequest();
    static void processPacket(uint8_t data[], uint8_t from, uint8_t to, bool debugEnabled);
    static void processPendingRequest();

public:
    I2CComms(Protocol* proto);

    void begin();
    void loop();
};
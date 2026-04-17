#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "../protocol/Protocol.h"

#define SLAVE_ADDR 0x04

class I2CComms {
private:
    Protocol* protocol;
    static uint8_t inputBuffer[32];
    static uint8_t inputLen;

    static void onReceive(int len);
    static void onRequest();

public:
    I2CComms(Protocol* proto);

    void begin();
};
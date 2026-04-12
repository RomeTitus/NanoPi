#include "I2CComms.h"

// ----------------------------------------------------
// STATIC DEFINITIONS (required for Wire callbacks)
// ----------------------------------------------------
I2CResponseQueue I2CComms::queue;
uint8_t I2CComms::inputBuffer[32];
uint8_t I2CComms::inputLen = 0;

// We need a global pointer bridge for ISR safety
static Protocol* g_protocol = nullptr;

// ----------------------------------------------------
// CONSTRUCTOR
// ----------------------------------------------------
I2CComms::I2CComms(Protocol* proto) {
    protocol = proto;
    g_protocol = proto;
}

// ----------------------------------------------------
// START I2C SLAVE
// ----------------------------------------------------
void I2CComms::begin() {
    Wire.begin(SLAVE_ADDR);

    Wire.onReceive(I2CComms::onReceive);
    Wire.onRequest(I2CComms::onRequest);
}

// ----------------------------------------------------
// RECEIVE EVENT (MASTER → SLAVE)
// ----------------------------------------------------
void I2CComms::onReceive(int len) {

    inputLen = 0;

    while (Wire.available() && inputLen < 32) {
        inputBuffer[inputLen++] = Wire.read();
    }

    uint8_t response[32];
    uint8_t responseLen = 0;

    // ----------------------------------------------------
    // PROCESS THROUGH PROTOCOL
    // ----------------------------------------------------
    if (g_protocol != nullptr) {
        g_protocol->processBinary(inputBuffer,
                                  inputLen,
                                  response,
                                  responseLen);
    }

    // ----------------------------------------------------
    // ENQUEUE RESPONSE (IMPORTANT FOR TIMING)
    // ----------------------------------------------------
    if (responseLen > 0) {
        queue.enqueue(response, responseLen);
    }
}

// ----------------------------------------------------
// REQUEST EVENT (MASTER READS RESPONSE)
// ----------------------------------------------------
void I2CComms::onRequest() {

    uint8_t response[32];
    uint8_t len = 0;

    // If we have a queued response, send it
    if (queue.dequeue(response, len)) {
        Wire.write(response, len);
    }
    else {
        // Always respond (prevents I2C hang)
        uint8_t fallback[2] = {0xAA, 0x00};
        Wire.write(fallback, 2);
    }
}
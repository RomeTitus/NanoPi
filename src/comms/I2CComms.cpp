#include "I2CComms.h"

// ----------------------------------------------------
// STATIC DEFINITIONS (required for Wire callbacks)
// ----------------------------------------------------
uint8_t I2CComms::inputBuffer[32];
uint8_t I2CComms::inputLen = 0;
static uint8_t lastResponse[32];
static uint8_t lastResponseLen = 0;

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

    // Filter out register address byte from read_i2c_block_data() calls
    // These are single-byte transmissions and should not be processed as commands
    if (inputLen == 1) {
        return;
    }

    Serial.print("I2C Receive:");
    for (size_t i = 0; i < inputLen; i++)        {
        Serial.print(inputBuffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    uint8_t response[32];
    uint8_t responseLen = 0;

    // Process through protocol
    if (g_protocol != nullptr) {
        g_protocol->processBinary(inputBuffer,
                                  inputLen,
                                  response,
                                  responseLen);
    }

    // Store response for immediate transmission
    if (responseLen > 0) {
        Serial.print("I2C Response:");
        for (size_t i = 0; i < responseLen; i++)        {
            Serial.print(response[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        memcpy(lastResponse, response, responseLen);
        lastResponseLen = responseLen;
    }
}

// ----------------------------------------------------
// REQUEST EVENT (MASTER READS RESPONSE)
// ----------------------------------------------------
void I2CComms::onRequest() {
    
    Serial.print("I2C Actual Response:");
    for (size_t i = 0; i < lastResponseLen; i++)        {
        Serial.print(lastResponse[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // Send the last response immediately
    if (lastResponseLen > 0) {
        Wire.write(lastResponse, lastResponseLen);
        lastResponseLen = 0;  // Clear after sending
    }
    else {
        // Always respond (prevents I2C hang) 
        uint8_t fallback[2] = {0xAA, 0x00};
        Wire.write(fallback, 2);
    }
}
#include "I2CComms.h"

// ----------------------------------------------------
// STATIC DEFINITIONS (required for Wire callbacks)
// ----------------------------------------------------
uint8_t I2CComms::inputBuffer[I2CComms::INPUT_SIZE];

// Response queue for batched messages
uint8_t I2CComms::responseQueue[I2CComms::QUEUE_SIZE][I2CComms::RESPONSE_SIZE];
uint8_t I2CComms::queueCount = 0;

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
// Split by 0xAA delimiter and process batches
// ----------------------------------------------------
void I2CComms::onReceive(int len) {
    uint8_t inputLen = 0;

    while (Wire.available() && inputLen < I2CComms::INPUT_SIZE) {
        inputBuffer[inputLen++] = Wire.read();
    }
    
    if (inputLen == 1) 
        return;

    int start = -1;

    for (int i = 0; i < len; i++) {

        if (inputBuffer[i] == 0xAA) {

            // If we already had a packet start, process previous packet
            if (start != -1) {
                processPacket(inputBuffer, start, i);
            }

            // Start new packet
            start = i;
        }
    }

    // Process final packet
    if (start != -1) {
        processPacket(inputBuffer, start, len);
    }
}

// ----------------------------------------------------
// REQUEST EVENT (MASTER READS RESPONSE)
// Send flattened queued responses in order
// ----------------------------------------------------
void I2CComms::onRequest() {
    if (queueCount == 0) {
        // No queued responses, send fallback (prevents I2C hang)
        Serial.println("I2C Queue empty, sending fallback");
        uint8_t fallback[2] = {0xAA, 0x00};
        Wire.write(fallback, 2);
        return;
    }
    
    for (uint8_t i = 0; i < queueCount; i++) {
        Serial.print("Queue Response ");
        Serial.print(i);
        Serial.print(": ");
        for (int j = 0; j < RESPONSE_SIZE; j++) {
            Serial.print("0x");
            if (responseQueue[i][j] < 16) Serial.print("0");
            Serial.print(responseQueue[i][j], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    // Send all queued responses as a single flattened block
    Wire.write(&responseQueue[0][0], queueCount * RESPONSE_SIZE);
    
    // Clear queue after sending
    queueCount = 0;
}


void I2CComms::processPacket(uint8_t data[], uint8_t from, uint8_t to) {
    
    uint8_t msgLen = to - from;
    uint8_t response[I2CComms::RESPONSE_SIZE];
    uint8_t responseLen = 0;

    if (msgLen <= 0) {
        return;
    }

    g_protocol->processBinary(&data[from],
                            msgLen,
                            response,
                            responseLen);

    if (responseLen > 0) {
        // Add response to queue if space available
        if (queueCount >= QUEUE_SIZE) {
            Serial.println("ERROR: Response queue full!");
            return;
        }
        
        memcpy(responseQueue[queueCount], response, responseLen);
        queueCount++;
    }
    
    // Optional: debug output
    Serial.print("Processed packet: ");
    for (int i = 0; i < msgLen; i++) {
        Serial.print("0x");
        if (data[from + i] < 16) Serial.print("0");
        Serial.print(data[from + i], HEX);
        Serial.print(" ");
    }

    Serial.print("Response: ");
    for (int i = 0; i < responseLen; i++) {
        Serial.print("0x");
        if (response[i] < 16) Serial.print("0");
        Serial.print(response[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

}
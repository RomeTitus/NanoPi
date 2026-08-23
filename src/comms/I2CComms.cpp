#include "I2CComms.h"
#include "I2CWatchdog.h"
#include "../protocol/StatusCodes.h"

// ----------------------------------------------------
// STATIC DEFINITIONS (required for Wire callbacks)
// ----------------------------------------------------
uint8_t I2CComms::inputBuffer[I2CComms::INPUT_SIZE];
uint8_t I2CComms::pendingBuffer[I2CComms::INPUT_SIZE];

// Response queue for batched messages
uint8_t I2CComms::responseQueue[I2CComms::QUEUE_SIZE][I2CComms::RESPONSE_SIZE];
volatile uint8_t I2CComms::queueCount = 0;
volatile uint8_t I2CComms::requestFlags = 0;
volatile uint8_t I2CComms::pendingLen = 0;
volatile uint8_t I2CComms::pendingFlags = 0;
volatile bool I2CComms::requestPending = false;
volatile bool I2CComms::responseReady = false;

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

    I2CWatchdog::begin();
}

void I2CComms::loop() {
    processPendingRequest();

    I2CWatchdog::check();
}

uint8_t I2CComms::calculateChecksum(const uint8_t* data, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

bool I2CComms::isDebugEnabled() {
    return (requestFlags & 0x01) != 0;
}

void I2CComms::packResponseRecord(const uint8_t* response,
                                  uint8_t responseLen,
                                  uint8_t outRecord[RESPONSE_SIZE]) {
    outRecord[0] = 0xF0; // malformed/unknown
    outRecord[1] = 0x00;
    outRecord[2] = 0x00;
    outRecord[3] = 0x00;

    if (responseLen == 0) {
        outRecord[0] = 0xF1; // empty response
        return;
    }

    if (response[0] != 0xAA) {
        outRecord[0] = 0xF2; // invalid sync
        return;
    }

    // Error frame from protocol: [0xAA, errorCode]
    if (responseLen == 2) {
        outRecord[0] = response[1];
        return;
    }

    // Success: status + up to 3 data bytes (right-aligned)
    outRecord[0] = 0x00;
    uint8_t payloadLen = responseLen - 1;
    if (payloadLen > 3) {
        payloadLen = 3;
    }

    uint8_t start = 4 - payloadLen;
    for (uint8_t i = 0; i < payloadLen; i++) {
        outRecord[start + i] = response[1 + i];
    }
}



// ----------------------------------------------------
// RECEIVE EVENT (MASTER → SLAVE)
// Incoming frame: [flags][payload...]
// Payload may contain multiple variable-length packets delimited by 0xAA.
// ----------------------------------------------------
void I2CComms::onReceive(int len) {
    (void)len;
    I2CWatchdog::noteActivity();

    //No Data
    if(len == 0) {
        return;
    }
    
    uint8_t inputLen = 0;
    while (Wire.available() && inputLen < I2CComms::INPUT_SIZE) {
        inputBuffer[inputLen++] = Wire.read();
    }
    
    // Drain any overflow bytes from Wire buffer.
    while (Wire.available()) {
        (void)Wire.read();
    }

    if (inputLen == 0) {
        return;
    }

    // Frame had only one control byte (common during SMBus read commands).
    // Ignore it and preserve any queued response.
    if (inputLen <= 1) {
        return;
    }

    requestFlags = inputBuffer[0];

    // New request arrived; discard any previous queued response so replies stay request-local.
    noInterrupts();
    queueCount = 0;
    responseReady = false;
    interrupts();

    // Store request and defer heavy processing to main loop.
    memcpy(pendingBuffer, inputBuffer, inputLen);
    noInterrupts();
    pendingLen = inputLen;
    pendingFlags = requestFlags;
    requestPending = true;
    responseReady = false;
    interrupts();
}

// ----------------------------------------------------
// REQUEST EVENT (MASTER READS RESPONSE)
// Outgoing frame: [count][records...][checksum]
// ----------------------------------------------------
void I2CComms::onRequest() {
    I2CWatchdog::noteActivity();

    const uint8_t MAX_I2C_TX = MAX_I2C_FRAME;
    uint8_t outBuffer[MAX_I2C_TX];
    uint8_t offset = 0;

    if (!responseReady) {
        // Busy/not-ready frame: one record with BUSY status.
        outBuffer[0] = 0x01;
        outBuffer[1] = StatusCodes::ERR_BUSY;
        outBuffer[2] = 0x00;
        outBuffer[3] = 0x00;
        outBuffer[4] = 0x00;
        outBuffer[5] = calculateChecksum(outBuffer, 5);
        Wire.write(outBuffer, 6);
        return;
    }

    if (queueCount > QUEUE_SIZE) {
        queueCount = QUEUE_SIZE;
    }

    outBuffer[offset++] = queueCount;

    for (uint8_t i = 0; i < queueCount; i++) {
        // Keep one byte for checksum.
        if ((uint8_t)(offset + RESPONSE_SIZE) > (MAX_I2C_TX - 1)) {
            break;
        }

        memcpy(&outBuffer[offset], responseQueue[i], RESPONSE_SIZE);
        offset += RESPONSE_SIZE;
    }

    // Update count if we truncated records to fit TX size.
    outBuffer[0] = (offset - 1) / RESPONSE_SIZE;

    uint8_t checksum = calculateChecksum(outBuffer, offset);
    outBuffer[offset++] = checksum;
    Wire.write(outBuffer, offset);

    if (isDebugEnabled()) {
        Serial.print("I2C TX frame: ");
        for (uint8_t i = 0; i < offset; i++) {
            Serial.print("0x");
            if (outBuffer[i] < 16) Serial.print("0");
            Serial.print(outBuffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    // Clear queue after sending
    queueCount = 0;
    responseReady = false;
}


void I2CComms::processPendingRequest() {
    if (!requestPending) {
        return;
    }

    uint8_t localBuffer[INPUT_SIZE];
    uint8_t localLen = 0;
    uint8_t localFlags = 0;

    noInterrupts();
    localLen = pendingLen;
    localFlags = pendingFlags;
    memcpy(localBuffer, pendingBuffer, localLen);
    requestPending = false;
    interrupts();

    requestFlags = localFlags;

    if (isDebugEnabled()) {
        Serial.print("I2C RX len=");
        Serial.print(localLen);
        Serial.print(" flags=0x");
        Serial.println(requestFlags, HEX);

        Serial.print("I2C RX data: ");
        for (uint8_t i = 0; i < localLen; i++) {
            Serial.print("0x");
            if (localBuffer[i] < 16) Serial.print("0");
            Serial.print(localBuffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    queueCount = 0;
    int start = -1;

    for (uint8_t i = 1; i < localLen; i++) {
        if (localBuffer[i] == 0xAA) {
            if (start != -1) {
                processPacket(localBuffer, start, i, isDebugEnabled());
            }
            start = i;
        }
    }

    if (start != -1) {
        processPacket(localBuffer, start, localLen, isDebugEnabled());
    } else if (queueCount < QUEUE_SIZE) {
        responseQueue[queueCount][0] = StatusCodes::ERR_FRAMING;
        responseQueue[queueCount][1] = 0x00;
        responseQueue[queueCount][2] = 0x00;
        responseQueue[queueCount][3] = 0x00;
        queueCount++;
    }

    noInterrupts();
    responseReady = true;
    interrupts();
}


void I2CComms::processPacket(uint8_t data[], uint8_t from, uint8_t to, bool debugEnabled) {
    uint8_t msgLen = to - from;
    uint8_t response[8];
    uint8_t responseLen = 0;

    if (msgLen == 0) {
        return;
    }

    g_protocol->processBinary(&data[from],
                            msgLen,
                            response,
                            responseLen,
                            debugEnabled);

    if (queueCount >= QUEUE_SIZE) {
        if (debugEnabled) {
            Serial.println("I2C response queue full");
        }
        return;
    }

    packResponseRecord(response, responseLen, responseQueue[queueCount]);

    if (debugEnabled) {
        Serial.print("Queued response status=0x");
        Serial.println(responseQueue[queueCount][0], HEX);
    }

    queueCount++;
}
#include "RS485Comms.h"
#include "RS485Config.h"
#include "../protocol/Protocol.h"

void RS485Comms::begin() {
    digitalWrite(RS485Config::DIRECTION_PIN, LOW);
    pinMode(RS485Config::DIRECTION_PIN, OUTPUT);
    bus.begin(RS485Config::BAUD);
}

void RS485Comms::send(uint8_t kind, uint16_t id, const uint8_t* data, uint8_t length) {
    uint8_t frame[15] = {0x55, kind, uint8_t(id), uint8_t(id >> 8), length};
    memcpy(frame + 5, data, length);
    uint16_t crc = RS485Frame::crc16(frame + 1, length + 4);
    frame[length + 5] = uint8_t(crc);
    frame[length + 6] = uint8_t(crc >> 8);
    digitalWrite(RS485Config::DIRECTION_PIN, HIGH);
    delayMicroseconds(10);
    // SoftwareSerial::write is synchronous, including the stop bit.
    // No hardware-UART flush is needed because there is no transmit buffer.
    bus.write(frame, length + 7);
    digitalWrite(RS485Config::DIRECTION_PIN, LOW);
}

bool RS485Comms::receive(RS485Frame::Packet& packet) {
    if (millis() - lastByteMs > 50) parser.reset();
    while (bus.available()) {
        lastByteMs = millis();
        if (parser.push(uint8_t(bus.read()), packet)) return true;
    }
    return false;
}

void RS485Comms::loop() {
    RS485Frame::Packet packet;
    if (!receive(packet) || packet.kind != RS485Frame::REQUEST) return;
    uint8_t response[8];
    uint8_t length = 0;
    // A wire request is local-only: never bounce a request back to its sender.
    protocol->processBinary(packet.payload, packet.length, response, length, false, false);
    delay(2); // Give the requester time to release DE and enable its receiver.
    send(RS485Frame::RESPONSE, packet.sequence, response, length);
}

bool RS485Comms::exchange(const uint8_t* request, uint8_t length,
                          uint8_t* response, uint8_t& responseLength) {
    responseLength = 0;
    if (length == 0 || length > RS485Frame::MAX_PAYLOAD) return false;
    while (bus.available()) (void)bus.read();
    parser.reset();
    ++sequence;
    send(RS485Frame::REQUEST, sequence, request, length);
    unsigned long started = millis();
    RS485Frame::Packet packet;
    while (millis() - started < RS485Config::RESPONSE_TIMEOUT_MS) {
        if (receive(packet) && packet.kind == RS485Frame::RESPONSE &&
            packet.sequence == sequence && packet.payload[0] == 0xAA &&
            (packet.length == 2 || packet.length == 3)) {
            memcpy(response, packet.payload, packet.length);
            responseLength = packet.length;
            return true;
        }
    }
    // No automatic retries: a lost acknowledgement must not repeat an output command.
    parser.reset();
    return false;
}

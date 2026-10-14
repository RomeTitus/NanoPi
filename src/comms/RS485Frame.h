#pragma once
#include <stdint.h>

// Fixed, bounded packets: 55, kind, sequence-low, sequence-high, length,
// payload (up to 8 bytes), CRC16-low, CRC16-high. CRC covers kind through payload.
namespace RS485Frame {
constexpr uint8_t MAX_PAYLOAD = 8;
constexpr uint8_t REQUEST = 1;
constexpr uint8_t RESPONSE = 2;
inline uint16_t crc16(const uint8_t* data, uint8_t length) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }
    return crc;
}
struct Packet {
    uint8_t kind;
    uint16_t sequence;
    uint8_t length;
    uint8_t payload[MAX_PAYLOAD];
};
class Parser {
    uint8_t bytes[15] = {};
    uint8_t used = 0;
public:
    void reset() { used = 0; }
    bool push(uint8_t value, Packet& packet) {
        if (used == 0 && value != 0x55) return false;
        bytes[used++] = value;
        if ((used == 2 && value != REQUEST && value != RESPONSE) ||
            (used == 5 && (value == 0 || value > MAX_PAYLOAD))) {
            reset();
            return false;
        }
        if (used < 5 || used != bytes[4] + 7) return false;
        uint8_t length = bytes[4];
        uint16_t received = bytes[5 + length] | (uint16_t(bytes[6 + length]) << 8);
        bool valid = received == crc16(bytes + 1, length + 4);
        if (valid) {
            packet.kind = bytes[1];
            packet.sequence = bytes[2] | (uint16_t(bytes[3]) << 8);
            packet.length = length;
            for (uint8_t i = 0; i < length; ++i) packet.payload[i] = bytes[5 + i];
        }
        reset();
        return valid;
    }
};
}

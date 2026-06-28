#pragma once

#include <Arduino.h>

namespace StatusCodes {
constexpr uint8_t OK = 0x00;

// Protocol-level errors returned in [0xAA, code] frames.
constexpr uint8_t ERR_BUSY = 0xFA;
constexpr uint8_t ERR_ULTRASONIC_TIMEOUT = 0xFB;
constexpr uint8_t ERR_FRAMING = 0xFC;
constexpr uint8_t ERR_UNKNOWN_COMMAND = 0xFD;
constexpr uint8_t ERR_VALIDATION = 0xFE;
}

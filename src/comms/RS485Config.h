#pragma once
#include <Arduino.h>

namespace RS485Config {
constexpr uint8_t PIN_OFFSET = 22;
constexpr uint8_t RX_PIN = 2;
constexpr uint8_t TX_PIN = 3;
constexpr uint8_t DIRECTION_PIN = 4;
constexpr unsigned long BAUD = 9600;
constexpr unsigned long RESPONSE_TIMEOUT_MS = 220;
inline bool reserved(uint8_t pin) {
    return pin == 0 || pin == 1 || pin == RX_PIN || pin == TX_PIN ||
        pin == DIRECTION_PIN || pin == 18 || pin == 19;
}
}

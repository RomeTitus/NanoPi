#pragma once

#include <Arduino.h>
#if defined(ARDUINO_ARCH_AVR)
#include <avr/wdt.h>
#endif

class I2CWatchdog {
private:
    static volatile unsigned long lastActivityMs;
    static volatile bool hasSeenActivity;

    static const unsigned long TIMEOUT_MS = 120000UL;

    static void feedHardwareWatchdog();

public:
    static void begin();
    static void noteActivity();
    static void check();

#ifdef UNIT_TEST
    static void setStateForTest(bool seenActivity, unsigned long lastActivity);
    static void setFeedHandlerForTest(void (*handler)());
#endif
};
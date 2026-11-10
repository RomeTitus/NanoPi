#pragma once

#include <Arduino.h>
#if defined(ARDUINO_ARCH_AVR)
#include <avr/wdt.h>
#endif

class I2CWatchdog {
private:
    static void feedHardwareWatchdog();

public:
    static void begin();
    static void check();

#ifdef UNIT_TEST
    static void setFeedHandlerForTest(void (*handler)());
#endif
};
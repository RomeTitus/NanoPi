#include "I2CWatchdog.h"

volatile unsigned long I2CWatchdog::lastActivityMs = 0;
volatile bool I2CWatchdog::hasSeenActivity = false;

#ifdef UNIT_TEST
static void (*g_watchdogFeedHandler)() = nullptr;
#endif

void I2CWatchdog::begin() {
#if defined(ARDUINO_ARCH_AVR)
    wdt_enable(WDTO_8S);
    wdt_reset();
#endif

    noInterrupts();
    lastActivityMs = millis();
    hasSeenActivity = false;
    interrupts();
}

void I2CWatchdog::noteActivity() {
    lastActivityMs = millis();
    hasSeenActivity = true;
}

void I2CWatchdog::check() {
    bool seenActivity = false;
    unsigned long lastActivity = 0;

    noInterrupts();
    seenActivity = hasSeenActivity;
    lastActivity = lastActivityMs;
    interrupts();

    if (!seenActivity || (unsigned long)(millis() - lastActivity) <= TIMEOUT_MS) {
        feedHardwareWatchdog();
    }
}

void I2CWatchdog::feedHardwareWatchdog() {
#ifdef UNIT_TEST
    if (g_watchdogFeedHandler != nullptr) {
        g_watchdogFeedHandler();
        return;
    }
#endif

#if defined(ARDUINO_ARCH_AVR)
    wdt_reset();
#endif
}

#ifdef UNIT_TEST
void I2CWatchdog::setStateForTest(bool seenActivity, unsigned long lastActivity) {
    noInterrupts();
    hasSeenActivity = seenActivity;
    lastActivityMs = lastActivity;
    interrupts();
}

void I2CWatchdog::setFeedHandlerForTest(void (*handler)()) {
    g_watchdogFeedHandler = handler;
}
#endif
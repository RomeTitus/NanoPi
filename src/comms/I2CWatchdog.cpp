#include "I2CWatchdog.h"

#ifdef UNIT_TEST
static void (*g_watchdogFeedHandler)() = nullptr;
#endif

void I2CWatchdog::begin() {
#if defined(ARDUINO_ARCH_AVR)
    wdt_enable(WDTO_8S);
    wdt_reset();
#endif

}

void I2CWatchdog::check() {
    // Feed when the main loop makes progress. An idle I2C bus is normal.
    feedHardwareWatchdog();
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
void I2CWatchdog::setFeedHandlerForTest(void (*handler)()) {
    g_watchdogFeedHandler = handler;
}
#endif
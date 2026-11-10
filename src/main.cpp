#include "devices/DeviceRegistry.h"
#include "devices/PinHandler.h"
#include "devices/UltrasonicHandler.h"
#include "protocol/Protocol.h"
#include "comms/RS485Comms.h"
#include "comms/SerialComms.h"
#include "comms/I2CComms.h"
#if defined(NANO_BENCH_NO_WATCHDOG) && defined(ARDUINO_ARCH_AVR)
#include <avr/wdt.h>
#endif

DeviceRegistry registry;

PinHandler pinHandler;
UltrasonicHandler usHandler;

Protocol protocol(&registry);

RS485Comms rs485Comms(&protocol);
SerialComms serialComms(&protocol);
I2CComms i2cComms(&protocol);

void setup() {
#if defined(NANO_BENCH_NO_WATCHDOG) && defined(ARDUINO_ARCH_AVR)
    MCUSR &= ~_BV(WDRF);
    wdt_disable();
#endif
    serialComms.begin();
    registry.registerCommand(0x01, "Pin", &pinHandler);
    registry.registerCommand(0x02, "US", &usHandler);

    protocol.setRemoteTransport(&rs485Comms);
    rs485Comms.begin();
    i2cComms.begin();
#ifdef NANO_BENCH_NO_WATCHDOG
    Serial.println(F("NanoPi BENCH-NO-WDT v1"));
#endif
}

void loop() {
    rs485Comms.loop();
    i2cComms.loop();
    serialComms.loop();
}

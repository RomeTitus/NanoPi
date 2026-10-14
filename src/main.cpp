#include "devices/DeviceRegistry.h"
#include "devices/PinHandler.h"
#include "devices/UltrasonicHandler.h"
#include "protocol/Protocol.h"
#include "comms/RS485Comms.h"
#include "comms/SerialComms.h"
#include "comms/I2CComms.h"

DeviceRegistry registry;

PinHandler pinHandler;
UltrasonicHandler usHandler;

Protocol protocol(&registry);

RS485Comms rs485Comms(&protocol);
SerialComms serialComms(&protocol);
I2CComms i2cComms(&protocol);

void setup() {
    serialComms.begin();
    registry.registerCommand(0x01, "Pin", &pinHandler);
    registry.registerCommand(0x02, "US", &usHandler);

    protocol.setRemoteTransport(&rs485Comms);
    rs485Comms.begin();
    i2cComms.begin();
}

void loop() {
    rs485Comms.loop();
    i2cComms.loop();
    serialComms.loop();
}

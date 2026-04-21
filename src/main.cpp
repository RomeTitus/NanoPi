#include "devices/DeviceRegistry.h"
#include "devices/PinHandler.h"
#include "devices/UltrasonicHandler.h"
#include "protocol/Protocol.h"
#include "comms/SerialComms.h"
#include "comms/I2CComms.h"

DeviceRegistry registry;

PinHandler pinHandler;
UltrasonicHandler usHandler;

Protocol protocol(&registry);

SerialComms serialComms(&protocol);
I2CComms i2cComms(&protocol);

void setup() {
    //Serial.begin(9600);

    registry.registerCommand(0x01, "Pin", &pinHandler);
    registry.registerCommand(0x02, "US", &usHandler);

    //serialComms.begin();
    i2cComms.begin();
}

void loop() {
    serialComms.loop();
}
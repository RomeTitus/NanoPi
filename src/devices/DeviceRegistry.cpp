#include "DeviceRegistry.h"

void DeviceRegistry::registerCommand(uint8_t cmd, IDevice* device) {
    commandIds[count] = cmd;
    devices[count] = device;
    count++;
}

IDevice* DeviceRegistry::getDevice(uint8_t cmd) {
    for (int i = 0; i < count; i++) {
        if (commandIds[i] == cmd) return devices[i];
    }
    return nullptr;
}
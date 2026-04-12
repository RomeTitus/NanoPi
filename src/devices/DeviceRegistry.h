#pragma once
#include "IDevice.h"

#define MAX_DEVICES 10

class DeviceRegistry {
private:
    uint8_t commandIds[MAX_DEVICES];
    IDevice* devices[MAX_DEVICES];
    uint8_t count = 0;

public:
    void registerCommand(uint8_t cmd, IDevice* device);
    IDevice* getDevice(uint8_t cmd);
};
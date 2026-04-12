#pragma once

#include <Arduino.h>
#include "IDevice.h"

#define MAX_DEVICES 10

struct RegistryEntry {
    uint8_t cmd;     // binary command ID (0x01, 0x02, ...)
    String text;     // text alias ("Pin", "US", ...)
    IDevice* device; // actual implementation
};

class DeviceRegistry {
private:
    RegistryEntry entries[MAX_DEVICES];
    uint8_t count = 0;

public:
    // Register BOTH binary + text mapping in one place
    void registerCommand(uint8_t cmd, String text, IDevice* device);

    // Binary lookup (used by Protocol binary path)
    IDevice* getDeviceByCmd(uint8_t cmd);

    // Text lookup (used by Protocol text → binary translation)
    IDevice* getDeviceByText(String text);

    // Convert text → binary command ID
    bool getCmdByText(String text, uint8_t& cmd);

    // Optional helper (useful for debugging / introspection later)
    uint8_t getCount();
};
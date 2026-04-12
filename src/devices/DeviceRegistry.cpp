#include "DeviceRegistry.h"

void DeviceRegistry::registerCommand(uint8_t cmd, String text, IDevice* device) {

    if (count >= MAX_DEVICES) {
        return; // silently fail (or you can assert/log later)
    }

    entries[count].cmd = cmd;
    entries[count].text = text;
    entries[count].device = device;

    count++;
}
// ----------------------------------------------------
// Binary lookup (0x01 → device)
// ----------------------------------------------------
IDevice* DeviceRegistry::getDeviceByCmd(uint8_t cmd) {

    for (int i = 0; i < count; i++) {
        if (entries[i].cmd == cmd) {
            return entries[i].device;
        }
    }

    return nullptr;
}

// ----------------------------------------------------
// Text lookup ("Pin" → device)
// ----------------------------------------------------
IDevice* DeviceRegistry::getDeviceByText(String text) {

    for (int i = 0; i < count; i++) {
        if (entries[i].text == text) {
            return entries[i].device;
        }
    }

    return nullptr;
}

// ----------------------------------------------------
// Text → command ID (used by Protocol translation)
// ----------------------------------------------------
bool DeviceRegistry::getCmdByText(String text, uint8_t& cmd) {
    for (int i = 0; i < count; i++) {
        if (entries[i].text == text) {
            cmd = entries[i].cmd;
            return true;
        }
    }

    return false;
}

// ----------------------------------------------------
uint8_t DeviceRegistry::getCount() {
    return count;
}
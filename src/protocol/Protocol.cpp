#include "Protocol.h"

Protocol::Protocol(DeviceRegistry* reg) {
    registry = reg;
}

String Protocol::processInput(String input) {
    return processText(input);
}

String Protocol::processText(String input) {
    String result = "";
    
    int start = 0;
    while (true) {
        int end = input.indexOf(';', start);
        if (end == -1) end = input.length();

        String cmd = input.substring(start, end);

        if (cmd.startsWith("Pin")) {
            IDevice* dev = registry->getDevice(0x01);
            result += dev->handleText(cmd);
        } else if (cmd.startsWith("US")) {
            IDevice* dev = registry->getDevice(0x02);
            result += dev->handleText(cmd);
        }

        if (end >= input.length()) break;
        result += ";";
        start = end + 1;
    }

    return result;
}

void Protocol::processBinary(uint8_t* data, uint8_t len, uint8_t* response, uint8_t& responseLen) {
    // Find 0xAA (handles misalignment)
    for (int i = 0; i < len; i++) {
        if (data[i] == 0xAA) {
            uint8_t cmd = data[i + 1];
            IDevice* dev = registry->getDevice(cmd);

            if (dev != nullptr) {
                dev->handleBinary(&data[i + 1], len - i - 1, response, responseLen);
            }
            return;
        }
    }
}
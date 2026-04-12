#include "Protocol.h"

Protocol::Protocol(DeviceRegistry* reg) {
    registry = reg;
}

// ------------------------------------------------------------
// PUBLIC ENTRY POINT
// ------------------------------------------------------------
String Protocol::processInput(String input) {

    // If it looks like binary payload (starts with 0xAA later handled separately)
    if (input.length() > 0 && (uint8_t)input[0] == 0xAA) {
        return "BINARY_NOT_SUPPORTED_AS_STRING";
    }

    return processTextBatch(input);
}

// ------------------------------------------------------------
// TEXT BATCH PROCESSING
// Example:
// Pin,2,1,1;US,9,10
// ------------------------------------------------------------
String Protocol::processTextBatch(String input) {

    String result = "";

    int start = 0;

    while (start < (int)input.length()) {

        int end = input.indexOf(';', start);
        if (end == -1) end = input.length();

        String token = input.substring(start, end);
        token.trim();

        if (token.length() > 0) {

            uint8_t binary[32];
            uint8_t len = 0;

            if (!translateToBinary(token, binary, len)) {
                result += "ERR";
            } else {
                processBinary(binary, len, nullptr, len);

                // Try to extract response if device wrote it
                if (len >= 3) {
                    int value = (binary[1] << 8) | binary[2];
                    result += String(value);
                } else {
                    result += "OK";
                }
            }
        }

        if (end >= (int)input.length()) break;
        result += ";";
        start = end + 1;
    }

    return result;
}

// ------------------------------------------------------------
// TEXT → BINARY TRANSLATION (uses DeviceRegistry ONLY)
// ------------------------------------------------------------
bool Protocol::translateToBinary(String input, uint8_t* out, uint8_t& len) {

    int commaIndex = input.indexOf(',');
    String key = (commaIndex == -1)
                    ? input
                    : input.substring(0, commaIndex);

    uint8_t cmd;

    if (!registry->getCmdByText(key, cmd)) {
        return false;
    }

    out[0] = 0xAA;
    out[1] = cmd;

    uint8_t idx = 2;

    int start = commaIndex + 1;

    while (start > 0 && start < (int)input.length()) {

        int next = input.indexOf(',', start);
        if (next == -1) next = input.length();

        String val = input.substring(start, next);
        val.trim();

        out[idx++] = (uint8_t)val.toInt();

        if (next >= (int)input.length()) break;
        start = next + 1;
    }

    len = idx;
    return true;
}

// ------------------------------------------------------------
// BINARY PROCESSING (0xAA SAFE SCAN)
// ------------------------------------------------------------
void Protocol::processBinary(uint8_t* data,
                              uint8_t len,
                              uint8_t* response,
                              uint8_t& responseLen) {

    responseLen = 0;

    for (int i = 0; i < len; i++) {

        // 🔥 MISALIGNMENT SAFE SYNC BYTE SEARCH
        if (data[i] == 0xAA) {

            if (i + 1 >= len) return;

            uint8_t cmd = data[i + 1];

            IDevice* dev = registry->getDeviceByCmd(cmd);

            if (!dev) {
                response[0] = 0xAA;
                response[1] = 0xFD; // unknown command
                responseLen = 2;
                return;
            }

            // ----------------------------------------------------
            // VALIDATION (DEVICE OWNED)
            // ----------------------------------------------------
            String error;

            if (!dev->validate(&data[i + 1], len - i - 1, error)) {
                response[0] = 0xAA;
                response[1] = 0xFE; // validation error
                responseLen = 2;
                return;
            }

            // ----------------------------------------------------
            // EXECUTION
            // ----------------------------------------------------
            dev->handleBinary(&data[i + 1],
                              len - i - 1,
                              response,
                              responseLen);

            return;
        }
    }

    // No sync byte found
    response[0] = 0xAA;
    response[1] = 0xFC; // framing error
    responseLen = 2;
}
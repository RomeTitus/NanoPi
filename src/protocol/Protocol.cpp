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
            uint8_t response[32];
            uint8_t responseLen = 0;

            if (!translateToBinary(token, binary, len)) {
                result += "ERR";
            } else {
                processBinary(binary, len, response, responseLen);

                // Append response bytes to result
                for (int i = 0; i < responseLen; i++) {
                    if (response[i] < 16) result += "0";
                    result += String(response[i], HEX);
                }
            }
        }

        if (end >= (int)input.length()) break;
        result += ";";
        start = end + 1;
    }

    return result;
}

// Helper function to convert parameter string to byte value(s)
// Returns the number of bytes written, 0 if invalid
uint8_t convertParam(String val, uint8_t* out) {
    val.trim();
    
    // Check for text keywords
    if (val == "On" || val == "on") { out[0] = 1; return 1; }
    if (val == "Off" || val == "off") { out[0] = 0; return 1; }
    if (val == "Read" || val == "read") { out[0] = 1; return 1; }
    if (val == "Write" || val == "write") { out[0] = 0; return 1; }
    
    // Check for pin format (e.g., "D2", "A6")
    if (val.length() >= 2 && isalpha(val[0])) {
        char pinType = val[0];
        String numStr = val.substring(1);
        
        // Validate that the rest is a number
        bool isNum = true;
        for (int i = 0; i < numStr.length(); i++) {
            if (!isdigit(numStr[i])) {
                isNum = false;
                break;
            }
        }
        
        if (isNum) {
            // Convert pin type letter to number: D=0, A=1
            if (pinType == 'D' || pinType == 'd') {
                out[0] = 0;
            } else if (pinType == 'A' || pinType == 'a') {
                out[0] = 1;
            } else {
                return 0; // Unknown pin type
            }
            out[1] = (uint8_t)numStr.toInt();
            return 2;
        }
    }
    
    // Default: try to convert as integer
    out[0] = (uint8_t)val.toInt();
    return 1;
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
        
        // Convert parameter and add bytes
        uint8_t paramBytes[2];
        uint8_t paramLen = convertParam(val, paramBytes);

        if (paramLen == 0) {
            return false;
        }
        
        for (int i = 0; i < paramLen; i++) {
            out[idx++] = paramBytes[i];
        }

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

        // MISALIGNMENT SAFE SYNC BYTE SEARCH
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
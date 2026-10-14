#include "Protocol.h"
#include "StatusCodes.h"
#include "../comms/RS485Config.h"

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
                processBinary(binary, len, response, responseLen, true);

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
        for (unsigned int i = 0; i < (unsigned int)numStr.length(); i++) {
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
            long number = numStr.toInt();
            if (number < 0 || number > 255) return 0;
            out[1] = (uint8_t)number;
            return 2;
        }
    }
    
    // Default: try to convert as integer
    if (val.length() == 0 || val.length() > 3) return 0;
    for (unsigned int i = 0; i < val.length(); ++i)
        if (!isdigit(val[i])) return 0;
    long number = val.toInt();
    if (number > 255) return 0;
    out[0] = (uint8_t)number;
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

        if (paramLen == 0 || idx + paramLen > 32) {
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
                              uint8_t& responseLen,
                              bool isDebugEnabled, bool allowRemote) {
    responseLen = 0;

    for (int i = 0; i < len; i++) {

        // MISALIGNMENT SAFE SYNC BYTE SEARCH
        if (data[i] == 0xAA) {

            if (i + 1 >= len) break; // Return ERR_FRAMING below for a lone sync byte.

            uint8_t cmd = data[i + 1];

            if (isDebugEnabled) {
                String deviceName;
                if (registry->getTextByCmd(cmd, deviceName)) {
                    Serial.print("processBinary cmd 0x");
                    Serial.print(cmd, HEX);
                    Serial.print(" -> ");
                    Serial.println(deviceName);
                } else {
                    Serial.print("processBinary cmd 0x");
                    Serial.print(cmd, HEX);
                    Serial.println(" -> unknown");
                }
            }

            IDevice* dev = registry->getDeviceByCmd(cmd);

            if (!dev) {
                response[0] = 0xAA;
                response[1] = StatusCodes::ERR_UNKNOWN_COMMAND;
                responseLen = 2;
                return;
            }

            // Only pin fields are offset, never modes or relay values.
            uint8_t routed[8];
            uint8_t* command = &data[i + 1];
            uint8_t commandLen = len - i - 1;
            bool forward = false;
            bool invalid = false;
            if (cmd == 0x01 || cmd == 0x02) {
                invalid = commandLen < 3 || commandLen > 4;
                if (!invalid) {
                    invalid = cmd == 0x02 ? commandLen != 3 :
                        (command[1] > 1 || commandLen != (command[1] == 0 ? 4 : 3));
                }
                if (!invalid) {
                    memcpy(routed, command, commandLen);
                    uint8_t first = cmd == 0x02 ? 1 : 2;
                    forward = command[first] >= RS485Config::PIN_OFFSET;
                    for (uint8_t p = first; p <= 2; ++p) {
                        if (command[p] >= 2 * RS485Config::PIN_OFFSET ||
                            (command[p] >= RS485Config::PIN_OFFSET) != forward) invalid = true;
                        if (forward) routed[p] -= RS485Config::PIN_OFFSET;
                    }
                    command = routed;
                }
                if (invalid || (forward && !allowRemote)) {
                    response[0] = 0xAA;
                    response[1] = StatusCodes::ERR_VALIDATION;
                    responseLen = 2;
                    return;
                }
            }

            // ----------------------------------------------------
            // VALIDATION (DEVICE OWNED)
            // ----------------------------------------------------
            String error;

            if (!dev->validate(command, commandLen, error)) {
                response[0] = 0xAA;
                response[1] = StatusCodes::ERR_VALIDATION;
                responseLen = 2;
                return;
            }

            if (forward) {
                uint8_t request[8] = {0xAA};
                memcpy(request + 1, command, commandLen);
                if (!remote || !remote->exchange(request, commandLen + 1, response, responseLen)) {
                    response[0] = 0xAA;
                    response[1] = StatusCodes::ERR_REMOTE_TIMEOUT;
                    responseLen = 2;
                }
                return;
            }
            
            // ----------------------------------------------------
            // EXECUTION
            // ----------------------------------------------------
            dev->handleBinary(command,
                              commandLen,
                              response,
                              responseLen,
                              isDebugEnabled);

            return;
        }
    }

    // No sync byte found
    response[0] = 0xAA;
    response[1] = StatusCodes::ERR_FRAMING;
    responseLen = 2;
}

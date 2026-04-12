#include "TextTranslator.h"

void TextTranslator::registerTranslation(String key, uint8_t cmd) {
    keys[count] = key;
    commands[count] = cmd;
    count++;
}

bool TextTranslator::translate(String input, uint8_t* out, uint8_t& outLen) {
    // Example: "Pin,2D,On"

    int commaIndex = input.indexOf(',');
    String commandKey = (commaIndex == -1) ? input : input.substring(0, commaIndex);

    for (int i = 0; i < count; i++) {
        if (commandKey == keys[i]) {
            out[0] = 0xAA;
            out[1] = commands[i];

            // VERY BASIC PARAM PARSING (extendable)
            uint8_t idx = 2;
            int start = commaIndex + 1;

            while (start > 0 && start < input.length()) {
                int next = input.indexOf(',', start);
                if (next == -1) next = input.length();

                String val = input.substring(start, next);

                out[idx++] = val.toInt(); // simple conversion

                if (next >= input.length()) break;
                start = next + 1;
            }

            outLen = idx;
            return true;
        }
    }

    return false;
}
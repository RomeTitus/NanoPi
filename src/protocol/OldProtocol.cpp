#include "Protocol.h"
#include "../devices/OldPinHandler.h"
#include "../devices/OldUltrasonicHandler.h"

void Protocol::processInput(uint8_t* data, int len, char* response) {

    int offset = 0;

    if (len > 1 && data[0] != 0xAA && data[1] == 0xAA) {
        offset = 1;
    }

    if (len > offset && data[offset] == 0xAA) {
        processBinary(data + offset, len - offset, (uint8_t*)response);
    } else {
        strncpy(response, "", BUFFER_SIZE);
        processText((char*)(data + offset), response);
    }
}

void Protocol::processBinary(uint8_t* req, int len, uint8_t* res) {

    uint8_t cmd = req[2];

    if (cmd == 0x01) {
        PinHandler::handle(req, res);
    }
    else if (cmd == 0x10) {
        UltrasonicHandler::handle(req, res);
    }
    else {
        res[0] = 0xBB;
        res[2] = 1;
    }
}

// ===== TEXT =====

void Protocol::processText(char* input, char* output) {

    char* saveptrText;
    char* cmd = strtok_r(input, ";", &saveptrText);

    while (cmd != NULL) {

        char single[16];
        processSingle(cmd, single);

        if (strlen(output) > 0) strcat(output, ";");
        strcat(output, single);

        cmd = strtok_r(NULL, ";", &saveptrText);
    }
}

void Protocol::processSingle(char* cmd, char* out) {

    char local[32];
    strncpy(local, cmd, sizeof(local));
    char* saveptrProccessSingle;
    char* token = strtok_r(local, ",", &saveptrProccessSingle);

    if (strcmp(token, "DEV") == 0) {
        UltrasonicHandler::handleText(local, out);
        return;
    }

    PinHandler::handleText(local, out);
}


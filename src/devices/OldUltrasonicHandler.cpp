#include "UltrasonicHandler.h"
#include <string.h>

Ultrasonic us[4];

uint16_t UltrasonicHandler::read(Ultrasonic &d) {

    digitalWrite(d.trig, LOW);
    delayMicroseconds(2);
    digitalWrite(d.trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(d.trig, LOW);

    long dur = pulseIn(d.echo, HIGH, 30000);
    return (dur * 0.0343) / 2;
}

void UltrasonicHandler::handle(uint8_t* req, uint8_t* res) {

    uint8_t cmdId = req[1];
    uint8_t action = req[3];
    uint8_t id = req[4];

    uint16_t result = 0;
    uint8_t status = 0;

    if (action == 0) {
        us[id].trig = req[5];
        us[id].echo = req[6];
        us[id].active = true;

        pinMode(req[5], OUTPUT);
        pinMode(req[6], INPUT);
    }
    else if (action == 1) {
        if (us[id].active) {
            result = read(us[id]);
        } else {
            status = 1;
            result = 0;
        }
    }

    res[0] = 0xBB;
    res[1] = cmdId;
    res[2] = status;
    res[3] = result >> 8;
    res[4] = result & 0xFF;
}

void UltrasonicHandler::handleText(char* cmd, char* out) {
    char* saveptrHandleText;
    char* dev = strtok_r(cmd, ",", &saveptrHandleText);
    char* devName = strtok_r(NULL, ",", &saveptrHandleText);
    char* action = strtok_r(NULL, ",", &saveptrHandleText);

    int id = devName[2] - '0';

    if (strcmp(action, "init") == 0) {
        int trig = atoi(strtok_r(NULL, ",", &saveptrHandleText));
        int echo = atoi(strtok_r(NULL, ",", &saveptrHandleText));

        us[id].trig = trig;
        us[id].echo = echo;
        us[id].active = true;

        pinMode(trig, OUTPUT);
        pinMode(echo, INPUT);

        strcpy(out, "OK");

    } else if (strcmp(action, "read") == 0) {
        int dist = read(us[id]);
        itoa(dist, out, 10);
    }
}
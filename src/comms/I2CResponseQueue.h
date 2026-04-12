#pragma once
#include <Arduino.h>

#define QUEUE_SIZE 5
#define MAX_PACKET 32

class I2CResponseQueue {
private:
    uint8_t buffer[QUEUE_SIZE][MAX_PACKET];
    uint8_t lengths[QUEUE_SIZE];

    uint8_t head = 0;
    uint8_t tail = 0;

public:
    bool enqueue(uint8_t* data, uint8_t len);
    bool dequeue(uint8_t* out, uint8_t& len);
    bool isEmpty();
};
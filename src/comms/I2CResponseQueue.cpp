#include "I2CResponseQueue.h"

bool I2CResponseQueue::enqueue(uint8_t* data, uint8_t len) {
    uint8_t next = (tail + 1) % QUEUE_SIZE;
    if (next == head) return false; // full

    memcpy(buffer[tail], data, len);
    lengths[tail] = len;
    tail = next;

    return true;
}

bool I2CResponseQueue::dequeue(uint8_t* out, uint8_t& len) {
    if (head == tail) return false;

    memcpy(out, buffer[head], lengths[head]);
    len = lengths[head];

    head = (head + 1) % QUEUE_SIZE;
    return true;
}

bool I2CResponseQueue::isEmpty() {
    return head == tail;
}
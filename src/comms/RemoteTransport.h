#pragma once
#include <Arduino.h>

class RemoteTransport {
public:
    virtual bool exchange(const uint8_t* request, uint8_t length,
                          uint8_t* response, uint8_t& responseLength) = 0;
};

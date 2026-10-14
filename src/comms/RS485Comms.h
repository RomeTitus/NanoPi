#pragma once
#include "RemoteTransport.h"
#include "RS485Frame.h"
#include "RS485Config.h"
#include <SoftwareSerial.h>
class Protocol;

class RS485Comms : public RemoteTransport {
    Protocol* protocol;
    SoftwareSerial bus;
    RS485Frame::Parser parser;
    uint16_t sequence = 0;
    unsigned long lastByteMs = 0;
    bool receive(RS485Frame::Packet& packet);
    void send(uint8_t kind, uint16_t id, const uint8_t* data, uint8_t length);
public:
    explicit RS485Comms(Protocol* proto) : protocol(proto),
        bus(RS485Config::RX_PIN, RS485Config::TX_PIN) {}
    void begin();
    void loop();
    bool exchange(const uint8_t* request, uint8_t length,
                  uint8_t* response, uint8_t& responseLength) override;
};

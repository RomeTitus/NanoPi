#pragma once
#include <Arduino.h>
#include "../devices/DeviceRegistry.h"
#include "../comms/RemoteTransport.h"

class Protocol {
private:
    DeviceRegistry* registry;
    RemoteTransport* remote = nullptr;

    bool translateToBinary(String input, uint8_t* out, uint8_t& len);
    String processTextBatch(String input);

public:
    Protocol(DeviceRegistry* reg);
    void setRemoteTransport(RemoteTransport* transport) { remote = transport; }

    String processInput(String input);

    void processBinary(uint8_t* data,
                       uint8_t len,
                       uint8_t* response,
                       uint8_t& responseLen,
                        bool isDebugEnabled, bool allowRemote = true);
};

#include <Arduino.h>
#include <unity.h>

#include "devices/DeviceRegistry.h"
#include "comms/RemoteTransport.h"
#include "protocol/Protocol.h"
#include "devices/UltrasonicHandler.h"

namespace {

class FakeRemote : public RemoteTransport {
public:
    uint8_t calls = 0;

    bool exchange(const uint8_t* data, uint8_t len, uint8_t* out,
                  uint8_t& outLen) override {
        (void)data;
        (void)len;
        ++calls;
        out[0] = 0xAA;
        out[1] = 0;
        out[2] = 123;
        outLen = 3;
        return true;
    }
};

void test_text_remote_and_overflow_validation() {
    DeviceRegistry registry;
    UltrasonicHandler device;
    registry.registerCommand(2, "US", &device);
    Protocol protocol(&registry);
    FakeRemote remote;
    protocol.setRemoteTransport(&remote);

    String validResult = protocol.processInput("US,31,32");
    TEST_ASSERT_EQUAL_STRING("aa007b", validResult.c_str());
    TEST_ASSERT_EQUAL_UINT8(1, remote.calls);

    String overflowResult = protocol.processInput("US,287,32");
    String negativeResult = protocol.processInput("US,-1,32");
    String malformedResult = protocol.processInput("US,bad,32");
    TEST_ASSERT_EQUAL_STRING("ERR", overflowResult.c_str());
    TEST_ASSERT_EQUAL_STRING("ERR", negativeResult.c_str());
    TEST_ASSERT_EQUAL_STRING("ERR", malformedResult.c_str());
    TEST_ASSERT_EQUAL_UINT8(1, remote.calls);
}

} // namespace

void setup() {
    delay(200);
    UNITY_BEGIN();
    RUN_TEST(test_text_remote_and_overflow_validation);
    UNITY_END();
}

void loop() {}

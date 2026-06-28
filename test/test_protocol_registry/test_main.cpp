#include <Arduino.h>
#include <unity.h>

#include "devices/DeviceRegistry.h"
#include "devices/IDevice.h"
#include "protocol/Protocol.h"
#include "protocol/StatusCodes.h"

namespace {

class FakeDeviceOk : public IDevice {
public:
    bool validate(uint8_t* data, uint8_t len, String& error) override {
        (void)data;
        (void)len;
        (void)error;
        return true;
    }

    void handleBinary(uint8_t* data,
                      uint8_t len,
                      uint8_t* response,
                      uint8_t& responseLen,
                      bool isDebugEnabled) override {
        (void)data;
        (void)len;
        (void)isDebugEnabled;
        response[0] = 0xAA;
        response[1] = 0x12;
        response[2] = 0x34;
        responseLen = 3;
    }
};

class FakeDeviceInvalid : public IDevice {
public:
    bool validate(uint8_t* data, uint8_t len, String& error) override {
        (void)data;
        (void)len;
        error = "invalid";
        return false;
    }

    void handleBinary(uint8_t* data,
                      uint8_t len,
                      uint8_t* response,
                      uint8_t& responseLen,
                      bool isDebugEnabled) override {
        (void)data;
        (void)len;
        (void)response;
        (void)responseLen;
        (void)isDebugEnabled;
    }
};

void test_registry_bidirectional_lookup() {
    DeviceRegistry registry;
    FakeDeviceOk device;

    registry.registerCommand(0x42, "Fake", &device);

    uint8_t cmd = 0;
    String text;

    TEST_ASSERT_EQUAL_UINT8(1, registry.getCount());
    TEST_ASSERT_TRUE(registry.getCmdByText("Fake", cmd));
    TEST_ASSERT_EQUAL_UINT8(0x42, cmd);
    TEST_ASSERT_TRUE(registry.getTextByCmd(0x42, text));
    TEST_ASSERT_EQUAL_STRING("Fake", text.c_str());
    TEST_ASSERT_EQUAL_PTR(&device, registry.getDeviceByCmd(0x42));
}

void test_processBinary_unknown_command() {
    DeviceRegistry registry;
    Protocol protocol(&registry);

    uint8_t req[] = {0xAA, 0x99};
    uint8_t res[8] = {0};
    uint8_t resLen = 0;

    protocol.processBinary(req, sizeof(req), res, resLen, false);

    TEST_ASSERT_EQUAL_UINT8(2, resLen);
    TEST_ASSERT_EQUAL_HEX8(0xAA, res[0]);
    TEST_ASSERT_EQUAL_HEX8(StatusCodes::ERR_UNKNOWN_COMMAND, res[1]);
}

void test_processBinary_validation_error() {
    DeviceRegistry registry;
    FakeDeviceInvalid device;
    registry.registerCommand(0x10, "Invalid", &device);

    Protocol protocol(&registry);

    uint8_t req[] = {0xAA, 0x10, 0x01};
    uint8_t res[8] = {0};
    uint8_t resLen = 0;

    protocol.processBinary(req, sizeof(req), res, resLen, false);

    TEST_ASSERT_EQUAL_UINT8(2, resLen);
    TEST_ASSERT_EQUAL_HEX8(0xAA, res[0]);
    TEST_ASSERT_EQUAL_HEX8(StatusCodes::ERR_VALIDATION, res[1]);
}

void test_processBinary_success_path() {
    DeviceRegistry registry;
    FakeDeviceOk device;
    registry.registerCommand(0x11, "Ok", &device);

    Protocol protocol(&registry);

    uint8_t req[] = {0xAA, 0x11, 0x01};
    uint8_t res[8] = {0};
    uint8_t resLen = 0;

    protocol.processBinary(req, sizeof(req), res, resLen, false);

    TEST_ASSERT_EQUAL_UINT8(3, resLen);
    TEST_ASSERT_EQUAL_HEX8(0xAA, res[0]);
    TEST_ASSERT_EQUAL_HEX8(0x12, res[1]);
    TEST_ASSERT_EQUAL_HEX8(0x34, res[2]);
}

void test_processBinary_framing_error() {
    DeviceRegistry registry;
    Protocol protocol(&registry);

    uint8_t req[] = {0x01, 0x02, 0x03};
    uint8_t res[8] = {0};
    uint8_t resLen = 0;

    protocol.processBinary(req, sizeof(req), res, resLen, false);

    TEST_ASSERT_EQUAL_UINT8(2, resLen);
    TEST_ASSERT_EQUAL_HEX8(0xAA, res[0]);
    TEST_ASSERT_EQUAL_HEX8(StatusCodes::ERR_FRAMING, res[1]);
}

} // namespace

void setup() {
    delay(200);
    UNITY_BEGIN();
    RUN_TEST(test_registry_bidirectional_lookup);
    RUN_TEST(test_processBinary_unknown_command);
    RUN_TEST(test_processBinary_validation_error);
    RUN_TEST(test_processBinary_success_path);
    RUN_TEST(test_processBinary_framing_error);
    UNITY_END();
}

void loop() {
}

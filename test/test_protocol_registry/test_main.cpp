#include <Arduino.h>
#include <unity.h>

#include "devices/DeviceRegistry.h"
#include "devices/IDevice.h"
#include "comms/I2CWatchdog.h"
#include "protocol/Protocol.h"
#include "protocol/StatusCodes.h"
#include "comms/RS485Frame.h"
#include "devices/PinHandler.h"
#include "devices/UltrasonicHandler.h"

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

volatile bool g_watchdogFeedCalled = false;

class FakeRemote : public RemoteTransport {
public:
    uint8_t calls = 0;
    uint8_t request[8] = {};
    uint8_t length = 0;
    bool succeeds = true;
    bool exchange(const uint8_t* data, uint8_t len, uint8_t* out, uint8_t& outLen) override {
        ++calls;
        length = len;
        memcpy(request, data, len);
        if (!succeeds) return false;
        out[0] = 0xAA; out[1] = 0; out[2] = 123; outLen = 3;
        return true;
    }
};

void test_remote_ultrasonic_subtracts_offset_and_preserves_reply() {
    DeviceRegistry registry;
    UltrasonicHandler device;
    registry.registerCommand(2, "US", &device);
    Protocol protocol(&registry);
    FakeRemote remote;
    protocol.setRemoteTransport(&remote);
    uint8_t req[] = {0xAA, 2, 31, 32}, res[8], len;
    uint8_t expected[] = {0xAA, 2, 9, 10};
    protocol.processBinary(req, sizeof(req), res, len, false);
    TEST_ASSERT_EQUAL_UINT8(1, remote.calls);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, remote.request, 4);
    TEST_ASSERT_EQUAL_UINT8(31, req[2]); // Do not mutate the input batch.
    TEST_ASSERT_EQUAL_UINT8(3, len);
    TEST_ASSERT_EQUAL_UINT8(123, res[2]);
}

void test_local_command_does_not_forward() {
    DeviceRegistry registry;
    FakeDeviceOk device;
    registry.registerCommand(2, "US", &device);
    Protocol protocol(&registry);
    FakeRemote remote;
    protocol.setRemoteTransport(&remote);
    uint8_t req[] = {0xAA, 2, 9, 10}, res[8], len;
    protocol.processBinary(req, sizeof(req), res, len, false);
    TEST_ASSERT_EQUAL_UINT8(0, remote.calls);
    TEST_ASSERT_EQUAL_UINT8(0x34, res[2]);
}

void test_bad_remote_routes_rejected() {
    DeviceRegistry registry;
    UltrasonicHandler device;
    registry.registerCommand(2, "US", &device);
    Protocol protocol(&registry);
    FakeRemote remote;
    protocol.setRemoteTransport(&remote);
    uint8_t requests[][4] = {
        {0xAA, 2, 9, 32}, {0xAA, 2, 31, 10}, // Mixed boards.
        {0xAA, 2, 44, 45}, {0xAA, 2, 255, 32}, // Outside both banks.
        {0xAA, 2, 24, 32}, {0xAA, 2, 31, 26}, // RS485 pins.
        {0xAA, 2, 40, 32}, {0xAA, 2, 31, 41}, // I2C pins.
        {0xAA, 2, 42, 43}, {0xAA, 2, 31, 31} // Analog-only / same pin.
    };
    for (uint8_t i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i) {
        uint8_t res[8], len;
        protocol.processBinary(requests[i], 4, res, len, false);
        TEST_ASSERT_EQUAL_UINT8(2, len);
        TEST_ASSERT_EQUAL_UINT8(StatusCodes::ERR_VALIDATION, res[1]);
    }
    TEST_ASSERT_EQUAL_UINT8(0, remote.calls);
}

void test_remote_request_cannot_forward_again() {
    DeviceRegistry registry;
    UltrasonicHandler device;
    registry.registerCommand(2, "US", &device);
    Protocol protocol(&registry);
    FakeRemote remote;
    protocol.setRemoteTransport(&remote);
    uint8_t req[] = {0xAA, 2, 31, 32}, res[8], len;
    protocol.processBinary(req, 4, res, len, false, false);
    TEST_ASSERT_EQUAL_UINT8(StatusCodes::ERR_VALIDATION, res[1]);
    TEST_ASSERT_EQUAL_UINT8(0, remote.calls);
}

void test_remote_timeout_is_explicit() {
    DeviceRegistry registry;
    UltrasonicHandler device;
    registry.registerCommand(2, "US", &device);
    Protocol protocol(&registry);
    FakeRemote remote;
    remote.succeeds = false;
    protocol.setRemoteTransport(&remote);
    uint8_t req[] = {0xAA, 2, 31, 32}, res[8], len;
    protocol.processBinary(req, 4, res, len, false);
    TEST_ASSERT_EQUAL_UINT8(2, len);
    TEST_ASSERT_EQUAL_UINT8(StatusCodes::ERR_REMOTE_TIMEOUT, res[1]);
}

void test_remote_pin_boundaries_and_write_value() {
    DeviceRegistry registry;
    PinHandler device;
    registry.registerCommand(1, "Pin", &device);
    Protocol protocol(&registry);
    FakeRemote remote;
    protocol.setRemoteTransport(&remote);
    uint8_t req[] = {0xAA, 1, 1, 43}, res[8], len;
    protocol.processBinary(req, 4, res, len, false);
    TEST_ASSERT_EQUAL_UINT8(21, remote.request[3]); // Remote A7 read.
    uint8_t write[] = {0xAA, 1, 0, 27, 1};
    protocol.processBinary(write, 5, res, len, false);
    uint8_t expected[] = {0xAA, 1, 0, 5, 1};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, remote.request, 5);
    req[3] = 22; // Remote UART RX is reserved.
    protocol.processBinary(req, 4, res, len, false);
    TEST_ASSERT_EQUAL_UINT8(StatusCodes::ERR_VALIDATION, res[1]);
    req[3] = 21; // Local A7 read validation remains valid.
    String error;
    TEST_ASSERT_TRUE(device.validate(req + 1, 3, error));
}

void test_rs485_crc_and_frame_validation() {
    const uint8_t vector[] = "123456789";
    TEST_ASSERT_EQUAL_HEX16(0x4B37, RS485Frame::crc16(vector, 9));
    uint8_t frame[] = {0x55, 1, 0x55, 0xAA, 4, 0xAA, 2, 9, 10, 0, 0};
    uint16_t crc = RS485Frame::crc16(frame + 1, 8);
    frame[9] = uint8_t(crc); frame[10] = uint8_t(crc >> 8);
    RS485Frame::Parser parser;
    RS485Frame::Packet packet;
    TEST_ASSERT_FALSE(parser.push(0x99, packet));
    for (uint8_t i = 0; i < 10; ++i) TEST_ASSERT_FALSE(parser.push(frame[i], packet));
    TEST_ASSERT_TRUE(parser.push(frame[10], packet));
    TEST_ASSERT_EQUAL_HEX16(0xAA55, packet.sequence);
    TEST_ASSERT_EQUAL_UINT8(4, packet.length);
    TEST_ASSERT_EQUAL_UINT8(10, packet.payload[3]);
    frame[8] ^= 1;
    for (uint8_t i = 0; i < sizeof(frame); ++i) TEST_ASSERT_FALSE(parser.push(frame[i], packet));
    uint8_t invalid[] = {0x55, 1, 0, 0, 255};
    for (uint8_t i = 0; i < sizeof(invalid); ++i) TEST_ASSERT_FALSE(parser.push(invalid[i], packet));
    frame[8] ^= 1;
    for (uint8_t i = 0; i < 10; ++i) TEST_ASSERT_FALSE(parser.push(frame[i], packet));
    TEST_ASSERT_TRUE(parser.push(frame[10], packet));
}

void markWatchdogFeedCalled() {
    g_watchdogFeedCalled = true;
}

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

void test_watchdog_feeds_without_i2c_traffic() {
    I2CWatchdog::setFeedHandlerForTest(markWatchdogFeedCalled);
    g_watchdogFeedCalled = false;

    I2CWatchdog::check();

    I2CWatchdog::setFeedHandlerForTest(nullptr);
    TEST_ASSERT_TRUE(g_watchdogFeedCalled);
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
    RUN_TEST(test_watchdog_feeds_without_i2c_traffic);
    RUN_TEST(test_remote_ultrasonic_subtracts_offset_and_preserves_reply);
    RUN_TEST(test_local_command_does_not_forward);
    RUN_TEST(test_bad_remote_routes_rejected);
    RUN_TEST(test_remote_request_cannot_forward_again);
    RUN_TEST(test_remote_timeout_is_explicit);
    RUN_TEST(test_remote_pin_boundaries_and_write_value);
    RUN_TEST(test_rs485_crc_and_frame_validation);
    UNITY_END();
}

void loop() {
}

#include <unity.h>
#include "command_ids.h"
#include "status_codes.h"
#include "command_map.h"
#include "ble_hid.h"

using namespace vamc;

class RetryTransport : public IBleHidTransport {
public:
    bool connected = true;
    uint8_t fail_first_n_sends = 0;
    uint8_t send_calls = 0;
    mutable uint8_t connection_checks = 0;
    uint8_t connect_on_check = 0;
    uint8_t sleep_calls = 0;
    uint32_t slept_ms[3] = {0, 0, 0};
    uint32_t now_ms = 100;

    bool is_connected() const override {
        ++connection_checks;
        return connected || (connect_on_check > 0 && connection_checks >= connect_on_check);
    }

    bool send_consumer_key(uint16_t) override {
        ++send_calls;
        now_ms += 20;
        return send_calls > fail_first_n_sends;
    }

    uint32_t get_time_ms() const override { return now_ms; }

    void sleep_ms(uint32_t duration_ms) override {
        if (sleep_calls < 3) slept_ms[sleep_calls] = duration_ms;
        ++sleep_calls;
        now_ms += duration_ms;
    }
};

RetryTransport g_transport;

void setUp() {
    g_transport = RetryTransport();
    ble_set_transport(&g_transport);
}

void tearDown() { ble_set_transport(nullptr); }

void test_TC_CMD_03_uses_three_configurable_exponential_retries() {
    g_transport.fail_first_n_sends = 3;
    const SendResult result = ble_send_hid_key(CommandId::PLAY);

    TEST_ASSERT_EQUAL(SendStatus::RETRIED, result.status);
    TEST_ASSERT_EQUAL_UINT8(4, g_transport.send_calls);
    TEST_ASSERT_EQUAL_UINT8(3, g_transport.sleep_calls);
    TEST_ASSERT_EQUAL_UINT32(kDefaultBackoffMs, g_transport.slept_ms[0]);
    TEST_ASSERT_EQUAL_UINT32(kDefaultBackoffMs * kRetryBackoffMultiplier,
                             g_transport.slept_ms[1]);
    TEST_ASSERT_EQUAL_UINT32(kDefaultBackoffMs * kRetryBackoffMultiplier *
                                 kRetryBackoffMultiplier,
                             g_transport.slept_ms[2]);
    TEST_ASSERT_EQUAL(BleError::BLE_NONE, ble_get_last_error());
}

void test_TC_CMD_03_rechecks_and_recovers_a_temporarily_missing_link() {
    g_transport.connected = false;
    g_transport.connect_on_check = 2;
    const SendResult result = ble_send_hid_key(CommandId::NEXT, 3);

    TEST_ASSERT_EQUAL(SendStatus::RETRIED, result.status);
    TEST_ASSERT_EQUAL_UINT8(1, g_transport.send_calls);
    TEST_ASSERT_EQUAL_UINT8(1, g_transport.sleep_calls);
    TEST_ASSERT_EQUAL_UINT32(kDefaultBackoffMs, g_transport.slept_ms[0]);
    TEST_ASSERT_EQUAL(BleError::BLE_NONE, ble_get_last_error());
}

void test_TC_CMD_03_reports_disconnected_after_three_retries() {
    g_transport.connected = false;
    const SendResult result = ble_send_hid_key(CommandId::PAUSE, 3);

    TEST_ASSERT_EQUAL(SendStatus::FAILED, result.status);
    TEST_ASSERT_EQUAL_UINT8(0, g_transport.send_calls);
    TEST_ASSERT_EQUAL_UINT8(4, g_transport.connection_checks);
    TEST_ASSERT_EQUAL_UINT8(3, g_transport.sleep_calls);
    TEST_ASSERT_EQUAL(BleError::BLE_NOT_CONNECTED, ble_get_last_error());
}

void test_TC_CMD_03_honors_a_smaller_retry_limit() {
    g_transport.fail_first_n_sends = 5;
    const SendResult result = ble_send_hid_key(CommandId::PAUSE, 1);

    TEST_ASSERT_EQUAL(SendStatus::FAILED, result.status);
    TEST_ASSERT_EQUAL_UINT8(2, g_transport.send_calls);
    TEST_ASSERT_EQUAL_UINT8(1, g_transport.sleep_calls);
    TEST_ASSERT_EQUAL(BleError::BLE_TIMEOUT, ble_get_last_error());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_CMD_03_uses_three_configurable_exponential_retries);
    RUN_TEST(test_TC_CMD_03_rechecks_and_recovers_a_temporarily_missing_link);
    RUN_TEST(test_TC_CMD_03_reports_disconnected_after_three_retries);
    RUN_TEST(test_TC_CMD_03_honors_a_smaller_retry_limit);
    return UNITY_END();
}

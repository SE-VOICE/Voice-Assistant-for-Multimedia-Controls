#include <unity.h>
#include "command_ids.h"
#include "status_codes.h"
#include "command_map.h"
#include "ble_hid.h"

using namespace vamc;

class MockBleTransport : public IBleHidTransport {
public:
    bool connected = true;
    uint32_t current_time = 100;
    uint32_t simulated_send_delay = 20; // 20ms per transmission
    uint8_t fail_first_n_attempts = 0;
    uint8_t send_calls = 0;
    uint16_t last_usage_code = 0;

    void reset() {
        connected = true;
        current_time = 100;
        simulated_send_delay = 20;
        fail_first_n_attempts = 0;
        send_calls = 0;
        last_usage_code = 0;
    }

    bool is_connected() const override {
        return connected;
    }

    bool send_consumer_key(uint16_t usage_code) override {
        last_usage_code = usage_code;
        current_time += simulated_send_delay;
        ++send_calls;
        if (send_calls <= fail_first_n_attempts) {
            return false;
        }
        return true;
    }

    uint32_t get_time_ms() const override {
        return current_time;
    }

    void sleep_ms(uint32_t ms) override {
        current_time += ms;
    }
};

MockBleTransport g_mock;

void setUp() {
    g_mock.reset();
    ble_set_transport(&g_mock);
}

void tearDown() {
    ble_set_transport(nullptr);
}

// Test TC-CMD-02: Transmit command as BLE HID media key within 2 s (VAMC-F-008)
void test_TC_CMD_02_successful_transmission_within_2s() {
    SendResult result = ble_send_hid_key(CommandId::VOLUME_UP, 0);

    TEST_ASSERT_EQUAL(SendStatus::SUCCESS, result.status);
    TEST_ASSERT_TRUE(result.latency_ms <= kMaxBleLatencyBudgetMs);
    TEST_ASSERT_EQUAL_HEX16(hid_usage::VOLUME_INCREMENT, g_mock.last_usage_code);
    TEST_ASSERT_EQUAL_UINT8(1, g_mock.send_calls);
}

void test_TC_CMD_02_all_7_commands_transmit_correct_hid_usages() {
    const CommandId commands[] = {
        CommandId::VOLUME_UP, CommandId::VOLUME_DOWN, CommandId::PAUSE,
        CommandId::RESUME, CommandId::PLAY, CommandId::NEXT, CommandId::PREVIOUS
    };

    for (auto cmd : commands) {
        g_mock.reset();
        SendResult res = ble_send_hid_key(cmd, 0);
        TEST_ASSERT_EQUAL(SendStatus::SUCCESS, res.status);
        TEST_ASSERT_TRUE(res.latency_ms <= 2000);
        TEST_ASSERT_EQUAL_HEX16(CommandMapper::to_hid_usage(cmd), g_mock.last_usage_code);
    }
}

// Test TC-CMD-03: BLE retry/backoff up to 3 times on temporary failure (VAMC-F-009)
void test_TC_CMD_03_retry_recovers_after_transient_failure() {
    g_mock.fail_first_n_attempts = 1; // Fails attempt 0, succeeds on attempt 1
    SendResult result = ble_send_hid_key(CommandId::PLAY, 3);

    TEST_ASSERT_EQUAL(SendStatus::RETRIED, result.status);
    TEST_ASSERT_TRUE(result.latency_ms <= kMaxBleLatencyBudgetMs);
    TEST_ASSERT_EQUAL_UINT8(2, g_mock.send_calls);
}

void test_TC_CMD_03_all_retries_exhausted_reports_failure() {
    g_mock.fail_first_n_attempts = 10; // Always fails
    SendResult result = ble_send_hid_key(CommandId::PAUSE, 3);

    TEST_ASSERT_EQUAL(SendStatus::FAILED, result.status);
    TEST_ASSERT_EQUAL_UINT8(4, g_mock.send_calls); // 1 initial + 3 retries = 4 attempts
    TEST_ASSERT_EQUAL(BleError::BLE_TIMEOUT, ble_get_last_error());
}

void test_disconnected_host_fails_immediately() {
    g_mock.connected = false;
    SendResult result = ble_send_hid_key(CommandId::NEXT, 3);

    TEST_ASSERT_EQUAL(SendStatus::FAILED, result.status);
    TEST_ASSERT_EQUAL_UINT8(0, g_mock.send_calls);
    TEST_ASSERT_EQUAL(BleError::BLE_NOT_CONNECTED, ble_get_last_error());
}

void test_latency_budget_exceeded_fails() {
    g_mock.simulated_send_delay = 2500; // Exceeds 2000ms budget
    SendResult result = ble_send_hid_key(CommandId::VOLUME_DOWN, 0);

    TEST_ASSERT_EQUAL(SendStatus::FAILED, result.status);
    TEST_ASSERT_TRUE(result.latency_ms > kMaxBleLatencyBudgetMs);
    TEST_ASSERT_EQUAL(BleError::BLE_TIMEOUT, ble_get_last_error());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_CMD_02_successful_transmission_within_2s);
    RUN_TEST(test_TC_CMD_02_all_7_commands_transmit_correct_hid_usages);
    RUN_TEST(test_TC_CMD_03_retry_recovers_after_transient_failure);
    RUN_TEST(test_TC_CMD_03_all_retries_exhausted_reports_failure);
    RUN_TEST(test_disconnected_host_fails_immediately);
    RUN_TEST(test_latency_budget_exceeded_fails);
    return UNITY_END();
}

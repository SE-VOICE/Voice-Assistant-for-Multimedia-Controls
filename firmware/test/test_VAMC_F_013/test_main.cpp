#include <unity.h>
#include "factory_reset.h"

using namespace vamc;

void setUp() {}
void tearDown() {}

class FakeStorage : public FactoryResetStorage {
public:
    bool wifi_ok = true;
    bool ble_ok = true;
    bool restart_ok = true;
    unsigned wifi_calls = 0;
    unsigned ble_calls = 0;
    unsigned restart_calls = 0;

    bool clear_wifi_credentials() override {
        ++wifi_calls;
        return wifi_ok;
    }
    bool clear_ble_pairing_data() override {
        ++ble_calls;
        return ble_ok;
    }
    bool restart_provisioning() override {
        ++restart_calls;
        return restart_ok;
    }
};

void test_TC_CONN_03_clears_both_stores_then_restarts_provisioning() {
    FakeStorage storage;
    FactoryReset reset(storage);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FactoryResetResult::SUCCESS),
                          static_cast<int>(reset.on_confirmed_long_press()));
    TEST_ASSERT_EQUAL_UINT(1, storage.wifi_calls);
    TEST_ASSERT_EQUAL_UINT(1, storage.ble_calls);
    TEST_ASSERT_EQUAL_UINT(1, storage.restart_calls);
}

void test_TC_CONN_03_clear_failure_still_attempts_both_and_blocks_restart() {
    FakeStorage storage;
    storage.wifi_ok = false;
    FactoryReset reset(storage);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FactoryResetResult::CLEAR_FAILED),
                          static_cast<int>(reset.on_confirmed_long_press()));
    TEST_ASSERT_EQUAL_UINT(1, storage.wifi_calls);
    TEST_ASSERT_EQUAL_UINT(1, storage.ble_calls);
    TEST_ASSERT_EQUAL_UINT(0, storage.restart_calls);

    FakeStorage other;
    other.ble_ok = false;
    FactoryReset other_reset(other);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FactoryResetResult::CLEAR_FAILED),
                          static_cast<int>(other_reset.on_confirmed_long_press()));
    TEST_ASSERT_EQUAL_UINT(1, other.wifi_calls);
    TEST_ASSERT_EQUAL_UINT(1, other.ble_calls);
    TEST_ASSERT_EQUAL_UINT(0, other.restart_calls);
}

void test_TC_CONN_03_restart_failure_is_reported() {
    FakeStorage storage;
    storage.restart_ok = false;
    FactoryReset reset(storage);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FactoryResetResult::PROVISIONING_RESTART_FAILED),
                          static_cast<int>(reset.on_confirmed_long_press()));
    TEST_ASSERT_EQUAL_UINT(1, storage.wifi_calls);
    TEST_ASSERT_EQUAL_UINT(1, storage.ble_calls);
    TEST_ASSERT_EQUAL_UINT(1, storage.restart_calls);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_CONN_03_clears_both_stores_then_restarts_provisioning);
    RUN_TEST(test_TC_CONN_03_clear_failure_still_attempts_both_and_blocks_restart);
    RUN_TEST(test_TC_CONN_03_restart_failure_is_reported);
    return UNITY_END();
}

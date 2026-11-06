#include <unity.h>
#include "ble_discovery.h"

using namespace vamc;
void setUp() {}
void tearDown() {}

class FakeAdvertisement : public BleAdvertisement {
public:
    bool start_ok = true;
    unsigned start_calls = 0;
    unsigned stop_calls = 0;
    bool start() override { ++start_calls; return start_ok; }
    void stop() override { ++stop_calls; }
};

class FakePresentation : public PairingPresentation {
public:
    bool visible = true;
    unsigned show_calls = 0;
    unsigned clear_calls = 0;
    bool show_passkey(uint32_t) override { ++show_calls; return visible; }
    void clear_passkey() override { ++clear_calls; }
};

class FakeRadio : public PairingRadio {
public:
    bool start_ok = true;
    unsigned start_calls = 0;
    unsigned erase_calls = 0;
    bool begin_passkey_pairing(uint32_t) override { ++start_calls; return start_ok; }
    void erase_untrusted_bond() override { ++erase_calls; }
};

struct Fixture {
    FakeAdvertisement advertisement;
    FakePresentation presentation;
    FakeRadio radio;
    AuthenticatedPairing pairing;
    BlePairingSetup setup;
    Fixture() : pairing(presentation, radio), setup(advertisement, pairing) {}
};

void test_TC_CONN_02_advertises_for_host_discovery() {
    Fixture f;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BleSetupResult::SUCCESS),
                          static_cast<int>(f.setup.start_discoverable()));
    TEST_ASSERT_TRUE(f.setup.advertising());
    TEST_ASSERT_EQUAL_UINT(1, f.advertisement.start_calls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BleSetupResult::ALREADY_ACTIVE),
                          static_cast<int>(f.setup.start_discoverable()));
    TEST_ASSERT_EQUAL_UINT(1, f.advertisement.start_calls);
}

void test_TC_CONN_02_pairs_after_discovery_using_displayed_passkey() {
    Fixture f;
    f.setup.start_discoverable();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BleSetupResult::SUCCESS),
                          static_cast<int>(f.setup.begin_pairing(123456)));
    TEST_ASSERT_EQUAL_UINT(1, f.presentation.show_calls);
    TEST_ASSERT_EQUAL_UINT(1, f.radio.start_calls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::AWAITING_HOST_CONFIRMATION),
                          static_cast<int>(f.setup.pairing_state()));
    f.setup.on_host_result(true, PairingMethod::PASSKEY);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::PAIRED),
                          static_cast<int>(f.setup.pairing_state()));
    TEST_ASSERT_FALSE(f.setup.advertising());
    TEST_ASSERT_EQUAL_UINT(1, f.advertisement.stop_calls);
}

void test_TC_CONN_02_cannot_pair_before_advertising() {
    Fixture f;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BleSetupResult::PAIRING_FAILED),
                          static_cast<int>(f.setup.begin_pairing(123456)));
    TEST_ASSERT_EQUAL_UINT(0, f.radio.start_calls);
}

void test_TC_CONN_02_advertising_failure_blocks_pairing() {
    Fixture f;
    f.advertisement.start_ok = false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BleSetupResult::ADVERTISING_FAILED),
                          static_cast<int>(f.setup.start_discoverable()));
    TEST_ASSERT_FALSE(f.setup.advertising());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BleSetupResult::PAIRING_FAILED),
                          static_cast<int>(f.setup.begin_pairing(123456)));
    TEST_ASSERT_EQUAL_UINT(0, f.radio.start_calls);
}

void test_TC_CONN_02_just_works_result_is_rejected_and_stopped() {
    Fixture f;
    f.setup.start_discoverable();
    f.setup.begin_pairing(123456);
    f.setup.on_host_result(true, PairingMethod::JUST_WORKS);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::REJECTED),
                          static_cast<int>(f.setup.pairing_state()));
    TEST_ASSERT_EQUAL_UINT(1, f.radio.erase_calls);
    TEST_ASSERT_FALSE(f.setup.advertising());
    TEST_ASSERT_EQUAL_UINT(1, f.advertisement.stop_calls);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_CONN_02_advertises_for_host_discovery);
    RUN_TEST(test_TC_CONN_02_pairs_after_discovery_using_displayed_passkey);
    RUN_TEST(test_TC_CONN_02_cannot_pair_before_advertising);
    RUN_TEST(test_TC_CONN_02_advertising_failure_blocks_pairing);
    RUN_TEST(test_TC_CONN_02_just_works_result_is_rejected_and_stopped);
    return UNITY_END();
}

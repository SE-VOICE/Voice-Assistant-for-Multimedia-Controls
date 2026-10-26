#include <unity.h>
#include "authenticated_pairing.h"

using namespace vamc;
void setUp() {}
void tearDown() {}

class FakePage : public PairingPresentation {
public:
    bool can_show = true;
    unsigned show_calls = 0;
    unsigned clear_calls = 0;
    uint32_t shown = 0;
    bool show_passkey(uint32_t passkey) override {
        ++show_calls;
        shown = passkey;
        return can_show;
    }
    void clear_passkey() override { ++clear_calls; }
};
class FakeRadio : public PairingRadio {
public:
    bool can_start = true;
    unsigned calls = 0;
    unsigned erase_calls = 0;
    uint32_t received = 0;
    bool begin_passkey_pairing(uint32_t passkey) override {
        ++calls;
        received = passkey;
        return can_start;
    }
    void erase_untrusted_bond() override { ++erase_calls; }
};

void test_TC_SEC_02_passkey_shown_before_authenticated_pairing() {
    FakePage page;
    FakeRadio radio;
    AuthenticatedPairing pairing(page, radio);
    TEST_ASSERT_TRUE(pairing.begin(123456));
    TEST_ASSERT_EQUAL_UINT(1, page.show_calls);
    TEST_ASSERT_EQUAL_UINT32(123456, page.shown);
    TEST_ASSERT_EQUAL_UINT(1, radio.calls);
    TEST_ASSERT_EQUAL_UINT32(123456, radio.received);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::AWAITING_HOST_CONFIRMATION),
                          static_cast<int>(pairing.state()));
    pairing.on_host_result(true, PairingMethod::PASSKEY);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::PAIRED),
                          static_cast<int>(pairing.state()));
    TEST_ASSERT_EQUAL_UINT(1, page.clear_calls);
}

void test_TC_SEC_02_just_works_and_failed_bond_are_rejected() {
    FakePage page;
    FakeRadio radio;
    AuthenticatedPairing pairing(page, radio);
    TEST_ASSERT_TRUE(pairing.begin(4));
    pairing.on_host_result(true, PairingMethod::JUST_WORKS);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::REJECTED),
                          static_cast<int>(pairing.state()));
    TEST_ASSERT_EQUAL_UINT(1, radio.erase_calls);
    TEST_ASSERT_EQUAL_UINT(1, page.clear_calls);
    TEST_ASSERT_TRUE(pairing.begin(42));
    pairing.on_host_result(false, PairingMethod::PASSKEY);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::REJECTED),
                          static_cast<int>(pairing.state()));
    TEST_ASSERT_EQUAL_UINT(1, radio.erase_calls);
}

void test_TC_SEC_02_unavailable_page_blocks_pairing() {
    FakePage page;
    page.can_show = false;
    FakeRadio radio;
    AuthenticatedPairing pairing(page, radio);
    TEST_ASSERT_FALSE(pairing.begin(123456));
    TEST_ASSERT_EQUAL_UINT(0, radio.calls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::REJECTED),
                          static_cast<int>(pairing.state()));
}

void test_TC_SEC_02_radio_failure_clears_display() {
    FakePage page;
    FakeRadio radio;
    radio.can_start = false;
    AuthenticatedPairing pairing(page, radio);
    TEST_ASSERT_FALSE(pairing.begin(123456));
    TEST_ASSERT_EQUAL_UINT(1, page.clear_calls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::REJECTED),
                          static_cast<int>(pairing.state()));
}

void test_TC_SEC_02_unexpected_events_and_invalid_passkey_do_not_pair() {
    FakePage page;
    FakeRadio radio;
    AuthenticatedPairing pairing(page, radio);
    pairing.on_host_result(true, PairingMethod::PASSKEY);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PairingState::IDLE),
                          static_cast<int>(pairing.state()));
    TEST_ASSERT_FALSE(pairing.begin(1000000U));
    TEST_ASSERT_EQUAL_UINT(0, page.show_calls);
    TEST_ASSERT_EQUAL_UINT(0, radio.calls);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_SEC_02_passkey_shown_before_authenticated_pairing);
    RUN_TEST(test_TC_SEC_02_just_works_and_failed_bond_are_rejected);
    RUN_TEST(test_TC_SEC_02_unavailable_page_blocks_pairing);
    RUN_TEST(test_TC_SEC_02_radio_failure_clears_display);
    RUN_TEST(test_TC_SEC_02_unexpected_events_and_invalid_passkey_do_not_pair);
    return UNITY_END();
}

#include <unity.h>
#include <type_traits>
#include "command_ids.h"
#include "status_codes.h"
#include "config.h"
#include "wake_word.h"
#include "asr_client.h"
#include "command_map.h"
#include "ble_hid.h"
#include "connectivity.h"
#include "diagnostics_ota.h"
#include "feedback.h"

using namespace vamc;
void setUp() {}
void tearDown() {}

class FakeFeedback : public Feedback {
public:
    unsigned calls = 0;
    FeedbackState last = FeedbackState::MUTED;
    void show(FeedbackState state) override { last = state; ++calls; }
};
class FakeLog : public DiagnosticLog {
public:
    unsigned calls = 0;
    LogEntry last{CommandId::PLAY, 0, 0};
    void append(const LogEntry& entry) override { last = entry; ++calls; }
};

void test_DR_01_feedback_can_be_replaced_without_hardware() {
    FakeFeedback fake;
    Feedback& output = fake;
    const FeedbackState states[] = {FeedbackState::LISTENING,
        FeedbackState::RECOGNIZED, FeedbackState::SUCCESS,
        FeedbackState::ERROR, FeedbackState::LOW_BATTERY, FeedbackState::MUTED};
    for (auto state : states) {
        output.show(state);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(state), static_cast<int>(fake.last));
    }
    TEST_ASSERT_EQUAL_UINT(6, fake.calls);
}
void test_DR_01_log_boundary_preserves_allowed_fields() {
    FakeLog fake;
    DiagnosticLog& log = fake;
    const LogEntry entry{CommandId::PREVIOUS, UINT32_MAX, UINT16_MAX};
    log.append(entry);
    TEST_ASSERT_EQUAL_UINT(1, fake.calls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(entry.command), static_cast<int>(fake.last.command));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, fake.last.timestamp);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, fake.last.error_code);
}
void test_DR_01_ble_contract_compiles_without_platform_headers() {
    using Expected = SendResult (*)(CommandId, uint8_t);
    TEST_ASSERT_TRUE((std::is_same<Expected, decltype(&ble_send_hid_key)>::value));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_DR_01_feedback_can_be_replaced_without_hardware);
    RUN_TEST(test_DR_01_log_boundary_preserves_allowed_fields);
    RUN_TEST(test_DR_01_ble_contract_compiles_without_platform_headers);
    return UNITY_END();
}

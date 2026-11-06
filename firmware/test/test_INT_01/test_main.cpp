#include <unity.h>
#include "command_dispatch.h"
#include "command_map.h"
using namespace vamc;
class FeedbackSpy : public Feedback {
public:
    FeedbackState last = FeedbackState::MUTED;
    unsigned calls = 0;
    void show(FeedbackState state) override { last = state; ++calls; }
};
class TransportSpy : public IBleHidTransport {
public:
    bool connected = true;
    bool succeeds = true;
    unsigned failures = 0, calls = 0;
    uint16_t usage = 0;
    uint32_t now = 0;
    bool is_connected() const override { return connected; }
    bool send_consumer_key(uint16_t code) override {
        usage = code; ++calls;
        return succeeds && calls > failures;
    }
    uint32_t get_time_ms() const override { return now; }
    void sleep_ms(uint32_t ms) override { now += ms; }
};
TransportSpy transport;
void setUp() { transport = TransportSpy(); ble_set_transport(&transport); }
void tearDown() { ble_set_transport(nullptr); }
void test_INT_01_all_seven_results_reach_matching_hid_usage() {
    const char* texts[] = {"volume up", "volume down", "pause", "resume", "play", "next", "previous"};
    const uint16_t usages[] = {0xe9, 0xea, 0xb1, 0xb0, 0xb0, 0xb5, 0xb6};
    FeedbackSpy feedback;
    CommandDispatch pipeline(feedback);
    for (unsigned i = 0; i < 7; ++i) {
        const DispatchResult result = pipeline.on_recognized_text(texts[i], 0.9f, i);
        TEST_ASSERT_TRUE(result.attempted);
        TEST_ASSERT_EQUAL(SendStatus::SUCCESS, result.transmission.status);
        TEST_ASSERT_EQUAL_HEX16(usages[i], transport.usage);
        TEST_ASSERT_EQUAL(FeedbackState::SUCCESS, feedback.last);
    }
    TEST_ASSERT_EQUAL_UINT(7, transport.calls);
}
void test_INT_01_invalid_ambiguous_low_confidence_send_nothing() {
    FeedbackSpy feedback;
    CommandDispatch pipeline(feedback);
    TEST_ASSERT_FALSE(pipeline.on_recognized_text("dishwasher", 0.9f, 0).attempted);
    TEST_ASSERT_FALSE(pipeline.on_recognized_text("pause next", 0.9f, 0).attempted);
    TEST_ASSERT_FALSE(pipeline.on_recognized_text("play", 0.1f, 0).attempted);
    TEST_ASSERT_FALSE(pipeline.on_recognized_text(nullptr, 0.9f, 0).attempted);
    TEST_ASSERT_EQUAL_UINT(0, transport.calls);
    TEST_ASSERT_EQUAL(FeedbackState::ERROR, feedback.last);
}
void test_INT_01_recognition_failure_send_nothing() {
    FeedbackSpy feedback;
    CommandDispatch pipeline(feedback);
    TEST_ASSERT_FALSE(pipeline.on_recognition_failure().attempted);
    TEST_ASSERT_EQUAL_UINT(0, transport.calls);
    TEST_ASSERT_EQUAL(FeedbackState::ERROR, feedback.last);
}
void test_INT_01_disconnected_host_is_error_not_success() {
    transport.connected = false;
    FeedbackSpy feedback;
    CommandDispatch pipeline(feedback);
    const DispatchResult result = pipeline.on_recognized_text("play", 0.9f, 0);
    TEST_ASSERT_TRUE(result.attempted);
    TEST_ASSERT_EQUAL(SendStatus::FAILED, result.transmission.status);
    TEST_ASSERT_EQUAL_UINT(0, transport.calls);
    TEST_ASSERT_EQUAL(FeedbackState::ERROR, feedback.last);
}
void test_INT_01_retry_success_is_success_feedback() {
    transport.failures = 1;
    FeedbackSpy feedback;
    CommandDispatch pipeline(feedback);
    const DispatchResult result = pipeline.on_recognized_text("pause", 0.9f, 0);
    TEST_ASSERT_EQUAL(SendStatus::RETRIED, result.transmission.status);
    TEST_ASSERT_EQUAL_UINT(2, transport.calls);
    TEST_ASSERT_EQUAL(FeedbackState::SUCCESS, feedback.last);
}
void test_INT_01_exhausted_transport_is_error_feedback() {
    transport.succeeds = false;
    FeedbackSpy feedback;
    CommandDispatch pipeline(feedback);
    const DispatchResult result = pipeline.on_recognized_text("next", 0.9f, 0);
    TEST_ASSERT_EQUAL(SendStatus::FAILED, result.transmission.status);
    TEST_ASSERT_EQUAL_UINT(4, transport.calls);
    TEST_ASSERT_EQUAL(FeedbackState::ERROR, feedback.last);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_INT_01_all_seven_results_reach_matching_hid_usage);
    RUN_TEST(test_INT_01_invalid_ambiguous_low_confidence_send_nothing);
    RUN_TEST(test_INT_01_recognition_failure_send_nothing);
    RUN_TEST(test_INT_01_disconnected_host_is_error_not_success);
    RUN_TEST(test_INT_01_retry_success_is_success_feedback);
    RUN_TEST(test_INT_01_exhausted_transport_is_error_feedback);
    return UNITY_END();
}

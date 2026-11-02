#include <unity.h>
#include <stdio.h>
#include "command_ids.h"
#include "command_gate.h"
#include "command_matcher.h"
#include "feedback.h"

using namespace vamc;

// Records every feedback state the gate shows.
class FakeFeedback : public Feedback {
public:
    unsigned calls = 0;
    FeedbackState last = FeedbackState::MUTED;
    void show(FeedbackState state) override {
        ++calls;
        last = state;
    }
};

static FakeFeedback feedback;

void setUp() { feedback = FakeFeedback(); }
void tearDown() {}

static void assert_rejected(const GateResult& r, GateDecision expected, const char* msg) {
    TEST_ASSERT_EQUAL_MESSAGE(expected, r.decision, msg);
    TEST_ASSERT_FALSE_MESSAGE(r.should_send(), msg);
}

// TC-ASR-03: an unsupported phrase gives error feedback and no command.
void test_TC_ASR_03_unrecognized_phrase_errors_and_sends_nothing() {
    CommandGate gate(feedback);
    const char* const phrases[] = {
        "what's the weather", "stop", "mute", "pause the dishwasher", "", "plan",
    };
    char msg[96];
    for (size_t i = 0; i < sizeof(phrases) / sizeof(phrases[0]); ++i) {
        feedback = FakeFeedback();
        snprintf(msg, sizeof(msg), "transcript: \"%s\"", phrases[i]);
        GateResult r = gate.evaluate_transcript(phrases[i], 0.9f, 1000);
        assert_rejected(r, GateDecision::REJECT_UNRECOGNIZED, msg);
        TEST_ASSERT_EQUAL_UINT_MESSAGE(1, feedback.calls, msg);
        TEST_ASSERT_EQUAL_MESSAGE(FeedbackState::ERROR, feedback.last, msg);
    }
}

// TC-ASR-03: an ambiguous phrase gives error feedback and no command.
void test_TC_ASR_03_ambiguous_phrase_errors_and_sends_nothing() {
    CommandGate gate(feedback);
    const char* const phrases[] = {"pause next", "play the next song", "volume up volume down"};
    char msg[96];
    for (size_t i = 0; i < sizeof(phrases) / sizeof(phrases[0]); ++i) {
        feedback = FakeFeedback();
        snprintf(msg, sizeof(msg), "transcript: \"%s\"", phrases[i]);
        GateResult r = gate.evaluate_transcript(phrases[i], 0.9f, 1000);
        assert_rejected(r, GateDecision::REJECT_AMBIGUOUS, msg);
        TEST_ASSERT_EQUAL_UINT_MESSAGE(1, feedback.calls, msg);
        TEST_ASSERT_EQUAL_MESSAGE(FeedbackState::ERROR, feedback.last, msg);
    }
}

void test_null_transcript_is_rejected() {
    CommandGate gate(feedback);
    assert_rejected(gate.evaluate_transcript(nullptr, 0.9f, 0),
                    GateDecision::REJECT_UNRECOGNIZED, "null");
    TEST_ASSERT_EQUAL(FeedbackState::ERROR, feedback.last);
}

// A recognised command passes with its timestamp and no error feedback.
void test_matched_command_is_sent_without_error_feedback() {
    CommandGate gate(feedback);
    GateResult r = gate.evaluate_transcript("turn it up", 0.92f, 4321);
    TEST_ASSERT_TRUE(r.should_send());
    TEST_ASSERT_EQUAL(GateDecision::SEND, r.decision);
    TEST_ASSERT_EQUAL(CommandId::VOLUME_UP, r.request.command);
    TEST_ASSERT_EQUAL_UINT32(4321, r.request.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, feedback.calls);
}

void test_all_7_commands_pass_the_gate() {
    CommandGate gate(feedback);
    const struct { const char* t; CommandId id; } cases[] = {
        {"volume up", CommandId::VOLUME_UP}, {"volume down", CommandId::VOLUME_DOWN},
        {"pause", CommandId::PAUSE},         {"resume", CommandId::RESUME},
        {"play", CommandId::PLAY},           {"next", CommandId::NEXT},
        {"previous", CommandId::PREVIOUS},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        GateResult r = gate.evaluate_transcript(cases[i].t, 0.8f, 1);
        TEST_ASSERT_TRUE_MESSAGE(r.should_send(), cases[i].t);
        TEST_ASSERT_EQUAL_MESSAGE(cases[i].id, r.request.command, cases[i].t);
    }
    TEST_ASSERT_EQUAL_UINT(0, feedback.calls);
}

// Low ASR confidence is treated as "not sure what you said".
void test_low_confidence_is_rejected() {
    CommandGate gate(feedback);
    GateResult r = gate.evaluate_transcript("next", 0.49f, 1);
    assert_rejected(r, GateDecision::REJECT_LOW_CONFIDENCE, "0.49");
    TEST_ASSERT_EQUAL(FeedbackState::ERROR, feedback.last);

    TEST_ASSERT_TRUE(gate.evaluate_transcript("next", 0.5f, 1).should_send());
    TEST_ASSERT_TRUE(gate.evaluate_transcript("next", 1.0f, 1).should_send());
}

void test_threshold_is_configurable() {
    CommandGate strict(feedback, 0.8f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.8f, strict.min_confidence());
    assert_rejected(strict.evaluate_transcript("pause", 0.79f, 1),
                    GateDecision::REJECT_LOW_CONFIDENCE, "strict 0.79");
    TEST_ASSERT_TRUE(strict.evaluate_transcript("pause", 0.8f, 1).should_send());
}

// Providers may omit confidence; that alone must not block a clear command.
void test_missing_confidence_is_not_checked() {
    CommandGate gate(feedback);
    GateResult r = gate.evaluate_transcript("pause", gate_limits::kNoConfidence, 7);
    TEST_ASSERT_TRUE(r.should_send());
    TEST_ASSERT_EQUAL(CommandId::PAUSE, r.request.command);
}

// Garbage confidence values fail closed.
void test_invalid_confidence_values_are_rejected() {
    CommandGate gate(feedback);
    volatile float zero = 0.0f;
    const float bad[] = {1.5f, -0.5f, zero / zero /* NaN */};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        assert_rejected(gate.evaluate_transcript("pause", bad[i], 1),
                        GateDecision::REJECT_LOW_CONFIDENCE, "bad confidence");
    }
}

// Confidence never rescues an unrecognised or ambiguous transcript.
void test_match_outcome_is_checked_before_confidence() {
    CommandGate gate(feedback);
    assert_rejected(gate.evaluate_transcript("pause next", 1.0f, 1),
                    GateDecision::REJECT_AMBIGUOUS, "ambiguous at 1.0");
    assert_rejected(gate.evaluate_transcript("weather", 1.0f, 1),
                    GateDecision::REJECT_UNRECOGNIZED, "unrecognized at 1.0");
}

// SAD 4.3: ASR errors (401, 429, 5xx, network) take the same error path.
void test_asr_failure_errors_and_sends_nothing() {
    CommandGate gate(feedback);
    GateResult r = gate.report_asr_failure();
    assert_rejected(r, GateDecision::REJECT_ASR_FAILURE, "asr failure");
    TEST_ASSERT_EQUAL_UINT(1, feedback.calls);
    TEST_ASSERT_EQUAL(FeedbackState::ERROR, feedback.last);
}

// Works with the real feedback controller: error is red with a buzzer tone.
void test_error_uses_real_feedback_controller_pattern() {
    FeedbackController controller;
    CommandGate gate(controller);
    gate.evaluate_transcript("weather", 0.9f, 1);
    TEST_ASSERT_EQUAL(FeedbackState::ERROR, controller.get_last_state());
    TEST_ASSERT_TRUE(controller.get_last_color() == colors::RED);
    TEST_ASSERT_NOT_EQUAL(0, controller.get_last_tone_freq());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_ASR_03_unrecognized_phrase_errors_and_sends_nothing);
    RUN_TEST(test_TC_ASR_03_ambiguous_phrase_errors_and_sends_nothing);
    RUN_TEST(test_null_transcript_is_rejected);
    RUN_TEST(test_matched_command_is_sent_without_error_feedback);
    RUN_TEST(test_all_7_commands_pass_the_gate);
    RUN_TEST(test_low_confidence_is_rejected);
    RUN_TEST(test_threshold_is_configurable);
    RUN_TEST(test_missing_confidence_is_not_checked);
    RUN_TEST(test_invalid_confidence_values_are_rejected);
    RUN_TEST(test_match_outcome_is_checked_before_confidence);
    RUN_TEST(test_asr_failure_errors_and_sends_nothing);
    RUN_TEST(test_error_uses_real_feedback_controller_pattern);
    return UNITY_END();
}

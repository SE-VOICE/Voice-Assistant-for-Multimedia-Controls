#include <unity.h>
#include "feedback.h"

using namespace vamc;

class MockLedDriver : public ILedDriver {
public:
    RgbColor last_color = colors::OFF;
    unsigned set_color_calls = 0;
    unsigned turn_off_calls = 0;

    void reset() {
        last_color = colors::OFF;
        set_color_calls = 0;
        turn_off_calls = 0;
    }

    void set_color(RgbColor color) override {
        last_color = color;
        ++set_color_calls;
    }

    void turn_off() override {
        last_color = colors::OFF;
        ++turn_off_calls;
    }
};

class MockBuzzerDriver : public IBuzzerDriver {
public:
    uint16_t last_freq = 0;
    uint32_t last_duration = 0;
    unsigned play_tone_calls = 0;
    unsigned stop_calls = 0;

    void reset() {
        last_freq = 0;
        last_duration = 0;
        play_tone_calls = 0;
        stop_calls = 0;
    }

    void play_tone(uint16_t freq_hz, uint32_t duration_ms) override {
        last_freq = freq_hz;
        last_duration = duration_ms;
        ++play_tone_calls;
    }

    void stop() override {
        last_freq = 0;
        last_duration = 0;
        ++stop_calls;
    }
};

MockLedDriver g_led;
MockBuzzerDriver g_buzzer;
FeedbackController g_controller(&g_led, &g_buzzer);

void setUp() {
    g_led.reset();
    g_buzzer.reset();
}

void tearDown() {}

// Test TC-CMD-04: LED/buzzer confirms success within 1s of command execution (VAMC-F-010)
void test_TC_CMD_04_success_feedback_executes_within_1s() {
    g_controller.show(FeedbackState::SUCCESS);

    // Verify AC-VAMC-F-010: confirms success within 1 s (1000 ms)
    uint32_t duration = g_controller.get_last_duration_ms();
    TEST_ASSERT_TRUE(duration <= kMaxSuccessFeedbackDurationMs);
    TEST_ASSERT_EQUAL_UINT32(200, duration);

    // Verify visual green confirmation
    TEST_ASSERT_EQUAL_UINT8(colors::GREEN.r, g_led.last_color.r);
    TEST_ASSERT_EQUAL_UINT8(colors::GREEN.g, g_led.last_color.g);
    TEST_ASSERT_EQUAL_UINT8(colors::GREEN.b, g_led.last_color.b);
    TEST_ASSERT_EQUAL_UINT(1, g_led.set_color_calls);

    // Verify audible chime confirmation
    TEST_ASSERT_EQUAL_UINT16(2000, g_buzzer.last_freq);
    TEST_ASSERT_EQUAL_UINT32(150, g_buzzer.last_duration);
    TEST_ASSERT_EQUAL_UINT(1, g_buzzer.play_tone_calls);
}

void test_listening_state_feedback() {
    g_controller.show(FeedbackState::LISTENING);

    TEST_ASSERT_EQUAL_UINT8(colors::BLUE.r, g_led.last_color.r);
    TEST_ASSERT_EQUAL_UINT8(colors::BLUE.g, g_led.last_color.g);
    TEST_ASSERT_EQUAL_UINT8(colors::BLUE.b, g_led.last_color.b);
    TEST_ASSERT_EQUAL_UINT(1, g_buzzer.stop_calls);
}

void test_recognized_state_feedback() {
    g_controller.show(FeedbackState::RECOGNIZED);

    TEST_ASSERT_EQUAL_UINT8(colors::YELLOW.r, g_led.last_color.r);
    TEST_ASSERT_EQUAL_UINT8(colors::YELLOW.g, g_led.last_color.g);
    TEST_ASSERT_EQUAL_UINT8(colors::YELLOW.b, g_led.last_color.b);
    TEST_ASSERT_EQUAL_UINT16(1200, g_buzzer.last_freq);
    TEST_ASSERT_EQUAL_UINT32(80, g_buzzer.last_duration);
}

void test_error_state_feedback() {
    g_controller.show(FeedbackState::ERROR);

    TEST_ASSERT_EQUAL_UINT8(colors::RED.r, g_led.last_color.r);
    TEST_ASSERT_EQUAL_UINT8(colors::RED.g, g_led.last_color.g);
    TEST_ASSERT_EQUAL_UINT8(colors::RED.b, g_led.last_color.b);
    TEST_ASSERT_EQUAL_UINT16(400, g_buzzer.last_freq);
    TEST_ASSERT_EQUAL_UINT32(250, g_buzzer.last_duration);
}

void test_low_battery_state_feedback() {
    g_controller.show(FeedbackState::LOW_BATTERY);

    TEST_ASSERT_EQUAL_UINT8(colors::ORANGE.r, g_led.last_color.r);
    TEST_ASSERT_EQUAL_UINT8(colors::ORANGE.g, g_led.last_color.g);
    TEST_ASSERT_EQUAL_UINT8(colors::ORANGE.b, g_led.last_color.b);
    TEST_ASSERT_EQUAL_UINT16(800, g_buzzer.last_freq);
}

void test_controller_handles_null_drivers_safely() {
    FeedbackController standalone(nullptr, nullptr);
    standalone.show(FeedbackState::SUCCESS);
    TEST_ASSERT_EQUAL(FeedbackState::SUCCESS, standalone.get_last_state());
    TEST_ASSERT_EQUAL_UINT32(200, standalone.get_last_duration_ms());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_CMD_04_success_feedback_executes_within_1s);
    RUN_TEST(test_listening_state_feedback);
    RUN_TEST(test_recognized_state_feedback);
    RUN_TEST(test_error_state_feedback);
    RUN_TEST(test_low_battery_state_feedback);
    RUN_TEST(test_controller_handles_null_drivers_safely);
    return UNITY_END();
}

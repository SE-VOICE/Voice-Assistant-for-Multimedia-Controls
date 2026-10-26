#include "feedback.h"

namespace vamc {

FeedbackController::FeedbackController(ILedDriver* led, IBuzzerDriver* buzzer)
    : led_driver_(led),
      buzzer_driver_(buzzer),
      last_state_(FeedbackState::MUTED),
      last_duration_ms_(0),
      last_color_(colors::OFF),
      last_tone_freq_(0) {}

void FeedbackController::set_led_driver(ILedDriver* led) {
    led_driver_ = led;
}

void FeedbackController::set_buzzer_driver(IBuzzerDriver* buzzer) {
    buzzer_driver_ = buzzer;
}

uint32_t FeedbackController::get_pattern_duration_ms(FeedbackState state) const {
    switch (state) {
        case FeedbackState::LISTENING:   return 0;   // Static steady indication
        case FeedbackState::RECOGNIZED:  return 80;  // Brief acknowledge chirp
        case FeedbackState::SUCCESS:     return 200; // Success flash & chime (well within 1000ms budget)
        case FeedbackState::ERROR:       return 300; // Warning beep & blink
        case FeedbackState::LOW_BATTERY: return 150; // Alert blip
        case FeedbackState::MUTED:       return 0;   // Static mute indication
        default:                         return 0;
    }
}

void FeedbackController::show(FeedbackState state) {
    last_state_ = state;
    last_duration_ms_ = get_pattern_duration_ms(state);

    RgbColor color = colors::OFF;
    uint16_t tone_freq = 0;
    uint32_t tone_dur = 0;

    switch (state) {
        case FeedbackState::LISTENING:
            color = colors::BLUE;
            tone_freq = 0;
            tone_dur = 0;
            break;

        case FeedbackState::RECOGNIZED:
            color = colors::YELLOW;
            tone_freq = 1200;
            tone_dur = 80;
            break;

        case FeedbackState::SUCCESS:
            color = colors::GREEN;
            tone_freq = 2000;
            tone_dur = 150;
            break;

        case FeedbackState::ERROR:
            color = colors::RED;
            tone_freq = 400;
            tone_dur = 250;
            break;

        case FeedbackState::LOW_BATTERY:
            color = colors::ORANGE;
            tone_freq = 800;
            tone_dur = 100;
            break;

        case FeedbackState::MUTED:
            color = colors::RED;
            tone_freq = 0;
            tone_dur = 0;
            break;
    }

    last_color_ = color;
    last_tone_freq_ = tone_freq;

    if (led_driver_ != nullptr) {
        if (color == colors::OFF) {
            led_driver_->turn_off();
        } else {
            led_driver_->set_color(color);
        }
    }

    if (buzzer_driver_ != nullptr) {
        if (tone_freq > 0) {
            buzzer_driver_->play_tone(tone_freq, tone_dur);
        } else {
            buzzer_driver_->stop();
        }
    }
}

} // namespace vamc

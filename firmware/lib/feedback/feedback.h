#pragma once

#include <stdint.h>
#include <stddef.h>

namespace vamc {

// Upper bound duration budget for command success feedback per VAMC-F-010 (<= 1 second)
constexpr uint32_t kMaxSuccessFeedbackDurationMs = 1000;

enum class FeedbackState : uint8_t {
    LISTENING, RECOGNIZED, SUCCESS, ERROR, LOW_BATTERY, MUTED
};

struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    bool operator==(const RgbColor& o) const {
        return r == o.r && g == o.g && b == o.b;
    }
    bool operator!=(const RgbColor& o) const {
        return !(*this == o);
    }
};

namespace colors {
    constexpr RgbColor OFF    = {0, 0, 0};
    constexpr RgbColor BLUE   = {0, 0, 255};
    constexpr RgbColor YELLOW = {255, 200, 0};
    constexpr RgbColor GREEN  = {0, 255, 0};
    constexpr RgbColor RED    = {255, 0, 0};
    constexpr RgbColor ORANGE = {255, 120, 0};
} // namespace colors

// Hardware abstraction interface for multi-color status LED
class ILedDriver {
public:
    virtual ~ILedDriver() = default;
    virtual void set_color(RgbColor color) = 0;
    virtual void turn_off() = 0;
};

// Hardware abstraction interface for audio buzzer
class IBuzzerDriver {
public:
    virtual ~IBuzzerDriver() = default;
    virtual void play_tone(uint16_t freq_hz, uint32_t duration_ms) = 0;
    virtual void stop() = 0;
};

// Abstract base class for feedback
class Feedback {
public:
    virtual ~Feedback() = default;
    virtual void show(FeedbackState state) = 0;
};

// Concrete feedback controller implementing multi-modal LED & buzzer patterns
class FeedbackController : public Feedback {
public:
    FeedbackController(ILedDriver* led = nullptr, IBuzzerDriver* buzzer = nullptr);

    void set_led_driver(ILedDriver* led);
    void set_buzzer_driver(IBuzzerDriver* buzzer);

    // Displays feedback for the requested state
    void show(FeedbackState state) override;

    // Returns the total duration of the feedback pattern for the specified state in ms
    uint32_t get_pattern_duration_ms(FeedbackState state) const;

    // Inspection helpers for diagnostics and testing
    FeedbackState get_last_state() const { return last_state_; }
    uint32_t get_last_duration_ms() const { return last_duration_ms_; }
    RgbColor get_last_color() const { return last_color_; }
    uint16_t get_last_tone_freq() const { return last_tone_freq_; }

private:
    ILedDriver* led_driver_;
    IBuzzerDriver* buzzer_driver_;
    FeedbackState last_state_;
    uint32_t last_duration_ms_;
    RgbColor last_color_;
    uint16_t last_tone_freq_;
};

} // namespace vamc

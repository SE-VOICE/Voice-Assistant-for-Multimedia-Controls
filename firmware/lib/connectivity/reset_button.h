#pragma once
#include <stdint.h>
namespace vamc {
// Caller supplies the team-approved duration; zero disables reset. No default
// duration is invented. Call poll regularly with a monotonic millisecond clock.
class ResetButton {
public:
    explicit ResetButton(uint32_t hold_ms) : hold_ms_(hold_ms) {}
    bool poll(bool pressed, uint32_t now_ms) {
        // A button held at boot is never treated as a reset request. Require
        // release first, which also resets the one-request-per-hold latch.
        if (!pressed) { armed_ = true; holding_ = false; return false; }
        if (!armed_ || !hold_ms_) return false;
        if (!holding_) { holding_ = true; started_ = now_ms; return false; }
        if (static_cast<uint32_t>(now_ms - started_) < hold_ms_) return false;
        armed_ = false;
        holding_ = false;
        return true;
    }
private:
    uint32_t hold_ms_;
    uint32_t started_ = 0;
    bool armed_ = false;
    bool holding_ = false;
};
}

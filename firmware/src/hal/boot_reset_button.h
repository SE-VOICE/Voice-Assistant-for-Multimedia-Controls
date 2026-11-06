#pragma once
#if defined(ARDUINO_ARCH_ESP32)
#include <Arduino.h>
#include "reset_button.h"
#include "factory_reset.h"
namespace vamc {
// Dhanush approved BOOT/GPIO0 on 7 November 2026. Do not hold BOOT while
// powering/resetting the board: GPIO0 is an ESP32 boot-strapping pin.
class BootResetButton {
public:
    BootResetButton(FactoryReset& reset, uint32_t approved_hold_ms = kApprovedBootResetHoldMs)
        : reset_(reset), button_(approved_hold_ms) {}
    void begin() { pinMode(0, INPUT_PULLUP); }
    bool poll(FactoryResetResult& result) {
        if (!button_.poll(digitalRead(0) == LOW, millis())) return false;
        result = reset_.on_confirmed_long_press();
        return true; // Caller must inspect result; an attempt is not success.
    }
private:
    FactoryReset& reset_;
    ResetButton button_;
};
}
#endif

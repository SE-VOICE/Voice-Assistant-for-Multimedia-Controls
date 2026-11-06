#pragma once
#include "command_gate.h"
#include "ble_hid.h"

namespace vamc {
struct DispatchResult {
    GateDecision decision;
    bool attempted;
    SendResult transmission;
};

// INT-01 downstream integration only. A real offline recognizer must supply
// its result; this does not capture audio, detect a wake word or run a model.
class CommandDispatch {
public:
    explicit CommandDispatch(Feedback& feedback,
        float minimum_confidence = gate_limits::kDefaultMinConfidence)
        : feedback_(feedback), gate_(feedback, minimum_confidence) {}
    DispatchResult on_recognized_text(const char* text, float confidence, uint32_t now_ms);
    DispatchResult on_recognition_failure();
private:
    Feedback& feedback_;
    CommandGate gate_;
};
}

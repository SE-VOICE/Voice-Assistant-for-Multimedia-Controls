#pragma once

#include "command_ids.h"
#include "command_matcher.h"
#include "feedback.h"
#include <stdint.h>

namespace vamc {

// VAMC-F-006: detect and report unrecognized or ambiguous commands without
// executing unintended actions.
//
// The gate is the single point between recognition and execution. Only a
// SEND decision carries a CommandRequest; every other decision shows the
// error LED/buzzer pattern and carries no command, so callers cannot
// accidentally transmit one.

enum class GateDecision : uint8_t {
    SEND,                   // exactly one command, confident enough
    REJECT_UNRECOGNIZED,    // no supported command in the transcript
    REJECT_AMBIGUOUS,       // two or more different commands
    REJECT_LOW_CONFIDENCE,  // ASR confidence below the threshold
    REJECT_ASR_FAILURE      // no transcript: network, auth, rate limit, 5xx
};

struct GateResult {
    GateDecision decision;
    CommandRequest request;  // meaningful only when decision == SEND

    bool should_send() const { return decision == GateDecision::SEND; }
};

namespace gate_limits {
    // Pending team review: transcripts whose ASR confidence is below this
    // value are rejected. Providers report confidence in [0.0, 1.0].
    constexpr float kDefaultMinConfidence = 0.5f;
    // Pass as `confidence` when the provider returned no confidence value.
    constexpr float kNoConfidence = -1.0f;
} // namespace gate_limits

class CommandGate {
public:
    explicit CommandGate(Feedback& feedback,
                         float min_confidence = gate_limits::kDefaultMinConfidence)
        : feedback_(feedback), min_confidence_(min_confidence) {}

    // Decide on an already-matched transcript. `confidence` is the ASR
    // confidence in [0, 1], or kNoConfidence when the provider gave none.
    // `now_ms` stamps the CommandRequest on SEND.
    GateResult evaluate(const MatchResult& match, float confidence, uint32_t now_ms);

    // Convenience: run the F-005 matcher on `transcript`, then evaluate.
    GateResult evaluate_transcript(const char* transcript, float confidence, uint32_t now_ms);

    // The ASR call produced no transcript. Always rejects with error feedback.
    GateResult report_asr_failure();

    float min_confidence() const { return min_confidence_; }

private:
    GateResult reject(GateDecision reason);

    Feedback& feedback_;
    float min_confidence_;
};

} // namespace vamc

#include "command_gate.h"

namespace vamc {

GateResult CommandGate::reject(GateDecision reason) {
    feedback_.show(FeedbackState::ERROR);
    GateResult r;
    r.decision = reason;
    r.request.command = CommandId::PAUSE;  // placeholder, never sent
    r.request.timestamp = 0;
    return r;
}

GateResult CommandGate::evaluate(const MatchResult& match, float confidence, uint32_t now_ms) {
    switch (match.outcome) {
        case MatchOutcome::MATCHED:
            break;
        case MatchOutcome::AMBIGUOUS:
            return reject(GateDecision::REJECT_AMBIGUOUS);
        case MatchOutcome::UNRECOGNIZED:
        default:
            return reject(GateDecision::REJECT_UNRECOGNIZED);
    }

    // A missing confidence (negative) is not checked. Anything else outside
    // [min, 1] — including NaN, which fails every comparison — is rejected.
    if (confidence != gate_limits::kNoConfidence &&
        !(confidence >= min_confidence_ && confidence <= 1.0f)) {
        return reject(GateDecision::REJECT_LOW_CONFIDENCE);
    }

    GateResult r;
    r.decision = GateDecision::SEND;
    r.request.command = match.command;
    r.request.timestamp = now_ms;
    return r;
}

GateResult CommandGate::evaluate_transcript(const char* transcript, float confidence,
                                            uint32_t now_ms) {
    return evaluate(CommandMatcher::match(transcript), confidence, now_ms);
}

GateResult CommandGate::report_asr_failure() {
    return reject(GateDecision::REJECT_ASR_FAILURE);
}

} // namespace vamc

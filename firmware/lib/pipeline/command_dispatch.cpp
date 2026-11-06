#include "command_dispatch.h"
namespace vamc {
DispatchResult CommandDispatch::on_recognized_text(const char* text, float confidence, uint32_t now_ms) {
    const GateResult gate = gate_.evaluate_transcript(text, confidence, now_ms);
    if (!gate.should_send()) return {gate.decision, false, {SendStatus::FAILED, 0}};
    feedback_.show(FeedbackState::RECOGNIZED);
    const SendResult sent = ble_send_hid_key(gate.request.command);
    feedback_.show(sent.status == SendStatus::FAILED ? FeedbackState::ERROR : FeedbackState::SUCCESS);
    return {gate.decision, true, sent};
}
DispatchResult CommandDispatch::on_recognition_failure() {
    return {gate_.report_asr_failure().decision, false, {SendStatus::FAILED, 0}};
}
}

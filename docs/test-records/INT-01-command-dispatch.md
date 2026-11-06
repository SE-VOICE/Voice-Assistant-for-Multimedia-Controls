# INT-01 — downstream integration evidence

Author: Dhanush S S. Independent cross-test: team, pending.

Approved target flow: Wake Word → Audio Capture → Offline Command Recognition
→ Command Mapping → Bluetooth HID → Connected Device.

`CommandDispatch` connects the existing public text gate, command mapper, BLE
sender/retry policy and feedback interfaces. It does not depend on internet or
a cloud client. The existing gate confidence parameter is preserved; it is not
a newly validated tinyML threshold. This downstream adapter accepts text results
for integration testing; the offline model's actual output contract must be
agreed with Harsh before connecting the model.

Six host integration cases verify all seven command usages, rejection without
transmission, recognition failure, disconnected host, successful retry and
exhausted retries. The BLE transport is fake. Tests do not demonstrate real
Bluetooth, wake-word detection, audio recognition or visible host media actions.

## Joint end-to-end test — actual result left blank

1. Record board, microphone, wake-word engine/phrase, offline model version,
   target host, firmware commit and independent testers.
2. Disable internet. Pair using the authenticated setup flow.
3. Speak the wake word and each of Play, Pause, Resume, Next, Previous,
   Volume Up and Volume Down. Observe the actual host action and feedback.
4. Check silence, unsupported speech, ambiguous/low-confidence results,
   disconnected host and retry failure produce no unintended media action.
5. Record timestamps and original SRS/STP latency/accuracy trials separately;
   fake transport timing is not a performance result.

Tester/date:
Firmware/model/microphone/host:
Observed results and evidence:
Limitations:
Independent acceptance / RTM status:

Status: partial integration software tested; full INT-01 acceptance pending.

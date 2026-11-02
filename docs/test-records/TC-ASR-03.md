# TC-ASR-03 — unrecognized/ambiguous command handling

Requirement: VAMC-F-006
Owner: Harsh Mannan
Independent tester: Chiranthan Shankar
Status: T (ready for native cross-test; not accepted)

## Part A — native (no hardware)

1. In `firmware`, run `pio test -e native -f test_VAMC_F_006`.
2. Confirm that unsupported phrases ("what's the weather", "stop", "mute"),
   ambiguous phrases ("pause next", "play the next song"), low-confidence
   transcripts and ASR failures each show the ERROR feedback state once and
   return no command (`should_send()` is false).
3. Confirm that all 7 supported commands pass the gate with no error feedback.

## Part B — device (blocked)

Blocked on VAMC-F-004 on hardware, the board selection, wake word and audio
capture. When unblocked: speak each unsupported and ambiguous phrase and verify
that the error LED/buzzer pattern plays and the host receives no media key.

Expected result from the STP: Error indication occurs and no unintended command is sent.

Actual result: __________
Test date: __________
Tester: __________
Board, host, and firmware version: __________
Evidence link: __________

Passing native tests does not fill in the actual result.

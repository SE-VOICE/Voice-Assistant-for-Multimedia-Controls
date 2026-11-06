# System Validation Report

**Project:** Voice Assistant for Multimedia Controls — Team 3  
**Prepared for:** Dhanush S S, Integration Lead  
**Execution date:** 07 November 2026, 01:50 IST (UTC+05:30)  
**Application source commit:** `91d5a74d815a2daff04afd896e4f135877d552b9`  
**Test mode:** author-run automated host tests; independent acceptance pending.

The course testing window shown in the brief is **2–6 November**. These tests
were actually run on **7 November 2026 IST**; results are not backdated. UTC
timestamps may fall on 6 November. Report date and evidence date are distinct
from the course deadline.

## Validation verdict

**FINAL SYSTEM VALIDATION INCOMPLETE — not ready for acceptance/freeze.**

The 93 C++ host cases and 10 optional Python cases passed. Six of the C++ cases
exercise downstream integration using a fake BLE transport. **No full
wake-word → microphone → offline WROOM recognition → Bluetooth HID → host action
trial was executed in this run.**

Of the **25 requirement-level STP cases**, **0 received new independent final
acceptance in this run**. This is an evidence boundary, not an assertion that
no earlier physical experiment ever occurred. Prior results require identifiable
test records before they can count toward acceptance. The 103 automated cases
are a different granularity and must not be reported as 103 STP cases passed.

## Environment and blockers observed

- Host: macOS/arm64, Python 3.13, Apple Clang/LLVM 21; exact versions in manifest.
- USB scan showed Bluetooth/debug/OnePlus ports and **no ESP32 serial device**.
- `pio run -d firmware -e esp32dev` failed with
  `UnknownEnvNamesError: Unknown environment names 'esp32dev'. Valid names are 'native'`.
- Main contains an inert Arduino entry point, wake-word/recognizer placeholders
  and portable/fake boundaries; the final target model and physical adapters
  still need integration. No completed on-board recognition engine is assumed.
- No live microphone, Bluetooth host media event, Windows dispatch, flash/security
  inspection, battery run or OTA update was performed. No device was flashed.
- No paid API or cloud speech service was used. The optional Python local-model
  path is development scope, not a replacement for WROOM acceptance.

Existing earlier Arduino compile smoke results are not rerun results here and
are not substituted for the missing configured full target build.

## Requirement traceability — current validation evidence

| Requirement / STP test | Owner → independent tester | Acceptance criterion | Evidence in this run / remaining validation |
|---|---|---|---|
| VAMC-F-001 / TC-WW-01 | G S Dheekshith → Harsh Mannan | Wake word spoken at normal volume within 2 m triggers the listening state. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-F-002 / TC-WW-02 | G S Dheekshith → Harsh Mannan | Capture microphone audio for up to 5 s in bounded reusable frames; do not require retaining a complete five-second waveform in RAM. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-F-003 / TC-WW-03 | G S Dheekshith → Harsh Mannan | Pressing mute disables audio capture until unmuted; LED shows mute state. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-F-004 / TC-ASR-01 | Harsh Mannan → Chiranthan Shankar | A validated target-board model returns a supported command class within 3 s after utterance end with internet unavailable; no audio is transmitted externally. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-F-005 / TC-ASR-02 | Harsh Mannan → Chiranthan Shankar | Every phrase in the approved test set maps to the correct one of volume up, volume down, pause, resume, play, next, or previous; variants are included in the fixed model vocabulary. | **Not accepted in this run.** 12 text-matcher tests; trained audio/model variants unverified |
| VAMC-F-006 / TC-ASR-03 | Harsh Mannan → Chiranthan Shankar | An unrecognized/ambiguous test phrase triggers error feedback and no command ID is sent to command mapping. | **Not accepted in this run.** 12 gate tests + downstream rejection tests; actual spoken rejection unverified |
| VAMC-F-007 / TC-CMD-01 | Chiranthan Shankar → Dhanush S S | All 7 commands independently verified against the host media player. | **Not accepted in this run.** 5 mapping tests + downstream seven-command test; host actions unverified |
| VAMC-F-008 / TC-CMD-02 | Chiranthan Shankar → Dhanush S S | Host receives and executes the key event within 2 s of recognition. | **Not accepted in this run.** 6 send-policy tests; real host execution/timing unverified |
| VAMC-F-009 / TC-CMD-03 | Chiranthan Shankar → Dhanush S S | 3 retries with backoff, then an error is indicated. | **Not accepted in this run.** 4 retry tests + downstream retry cases; real reconnect unverified |
| VAMC-F-010 / TC-CMD-04 | Chiranthan Shankar → Dhanush S S | LED/buzzer confirms success within 1 s of execution. | **Not accepted in this run.** 6 feedback tests; GPIO/buzzer and real timing unverified |
| VAMC-F-011 / TC-CONN-01 | Dhanush S S → G S Dheekshith | Device completes guided Wi-Fi provisioning within 5 min; voice recognition and BLE media commands remain independent of internet availability. | **Not accepted in this run.** 9 coordinator tests; real portal/Wi-Fi/timing unverified |
| VAMC-F-012 / TC-CONN-02 | Dhanush S S → G S Dheekshith | Device appears in the host's Bluetooth list and pairs successfully. | **Not accepted in this run.** 5 discovery/pairing tests; radio/host pairing unverified |
| VAMC-F-013 / TC-CONN-03 | Dhanush S S → G S Dheekshith | Long-press clears stored Wi-Fi/BLE data and restarts provisioning. | **Not accepted in this run.** 3 coordinator + 6 button cases; real erasure/reprovisioning unverified |
| VAMC-F-014 / TC-DIAG-01 | Chiranthan Shankar → Dhanush S S | Last N events retrievable over the serial/debug console; no audio or credentials logged. | **Not accepted in this run.** 5 logging tests; actual device serial retrieval unverified |
| VAMC-F-015 / TC-DIAG-02 | Chiranthan Shankar → Dhanush S S | New firmware image applied remotely without physical access. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-NF-001 / TC-PERF-01 | G S Dheekshith → Harsh Mannan | 90th percentile wake-word-to-action latency <= 3 s over 50 trials. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-NF-002 / TC-ACC-01 | Harsh Mannan → Chiranthan Shankar | Accuracy >= 90% over 100 test utterances in a quiet indoor environment. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-NF-003 / TC-UX-01 | Harsh Mannan → G S Dheekshith | Average setup time <= 5 min across 5 first-time users following the setup guide. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-NF-004 / TC-POWER-01 | G S Dheekshith → Harsh Mannan | Continuous idle-listening operation >= 8 h on a 2000 mAh battery. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-NF-005 / TC-MAINT-01 | G S Dheekshith → Dhanush S S | Code review confirms module boundaries; one module replaced in isolation without edits to others. | **Not accepted in this run.** Public interfaces present; prescribed replacement review not executed |
| VAMC-SR-001 / TC-SEC-01 | Harsh Mannan → Chiranthan Shankar | With internet unavailable, supported commands remain recognizable; network inspection shows no command audio or transcript is sent externally. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-SR-002 / TC-SEC-02 | Dhanush S S → G S Dheekshith | Pairing on a supporting host requires passkey/numeric confirmation, not 'Just Works'. | **Not accepted in this run.** 5 policy tests; actual host passkey flow unverified |
| VAMC-SR-003 / TC-SEC-03 | Dhanush S S → G S Dheekshith | Flash inspection after provisioning shows no plaintext credentials. | **Not accepted in this run.** 6 native OpenSSL vault tests; ESP32 flash/mbedTLS unverified |
| VAMC-SR-004 / TC-SEC-04 | G S Dheekshith → Harsh Mannan | Memory/flash inspection shows no retained audio after the session ends. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |
| VAMC-SR-005 / TC-SEC-05 | Chiranthan Shankar → Dhanush S S | Unsigned/tampered image is rejected before it is applied. | **Not accepted in this run.** No target acceptance execution; implementation/hardware/test environment still required |

NF-005 is a review/replacement requirement rather than a physical measurement;
its full prescribed review was not executed by this run. All other listed
device/system acceptance results remain unexecuted here. The table does not
overwrite the project's RTM status codes.

## Integration and release evidence

- INT-01: six host integration cases passed; seven command mappings, no-send
  rejection, recognition error, disconnected host, retry recovery/exhaustion tested.
  Full spoken-command/host evidence remains pending.
- INT-02: partial regression evidence only. All planned stories are not accepted;
  no release tag/freeze is created or justified by these reports.
- VID-01/VID-02: no new working-product videos were recorded/uploaded in this run.
- GitHub issue/PR snapshots are preserved in the evidence folder. Merged code and
  green checks do not imply original hardware or cross-test acceptance.

## Defects / blockers and completion plan

| Item | Responsible owner | Next action / evidence needed |
|---|---|---|
| Target build environment missing | Dhanush + Dheekshith | Approve/configure reproducible WROOM build; run it in CI |
| Microphone/wake path not validated | Dheekshith; Harsh cross-tests | Approved mic/pins/phrase/engine, real capture/wake/mute/cleanup records |
| Offline recognizer not validated | Harsh; Chiranthan cross-tests | Target model fit, actual command/variant/error/privacy and accuracy results |
| Real HID and feedback | Chiranthan; Dhanush cross-tests | Seven observed host actions, timing/retry, GPIO and serial evidence |
| Setup/pairing/storage/reset adapters | Dhanush; Dheekshith cross-tests | Real portal/passkey, flash inspection, boot unlock timing, actual BOOT reset |
| Performance/usability/power | Assigned owners in matrix | 50 latency trials, 100 utterances/SNR, 5 users, 8-hour battery run |
| OTA/signature tests | Chiranthan; Dhanush cross-tests | Approved keys/source/partitions; valid update and invalid-image rejection |
| Source baseline reconciliation | Harsh/DOC-01 + Dhanush review | Publish approved offline revisions, resolve stale source descriptions |
| Uncovered source paths | Respective module owners | Meaningful fault/protocol/platform tests, then rerun coverage |

Do not backfill actual results, claim a missing experiment passed, or change
dates to match the earlier course window. Record independent results when run.

## Scope and acceptance boundary

The approved project uses ESP32-WROOM and offline fixed-vocabulary recognition.
No live cloud API, microphone, flash operation or Bluetooth/media action was
performed by this report run. Passing host tests is not completion of the
physical SRS/STP acceptance criteria. This report does not set RTM A/F statuses,
close issues, complete INT-02, create a release tag or freeze development.

The locally revised SRS/SAD/STP/backlog implement Dhanush's offline redesign;
some revisions are still unpublished and remote source text remains inconsistent.
The exact source-document hashes are in [source-baselines.json](evidence/source-baselines.json).
Harsh/DOC-01 must reconcile and publish those baselines before final acceptance.
Legacy `TC-ASR-*` IDs are retained; their names do not authorize cloud recognition.

## Reproduction and evidence

From the repository root, with PlatformIO installed:

```bash
pio test -d firmware -e native
python3 -m pip install coverage==7.17.2
python3 scripts/measure_coverage.py --output /tmp/vamc-report-rerun
```

The coverage runner currently supports **macOS** with Xcode command-line tools,
Homebrew `openssl@3` and Unity installed by the PlatformIO test run. It compiles
the same existing C++ tests and library sources with Apple Clang/LLVM 21 at `-O0`,
using `-fprofile-instr-generate -fcoverage-mapping`. Test/Unity code is not
instrumented. All 12 portable `.cpp` objects are linked into every suite; the
15 raw profiles are merged once, and one binary provides their shared mappings.
This avoids counting shared code 15 times. The runner then invokes Python
coverage with `--branch`, including all six prototype source files and excluding
its tests. It records tool versions, commands, source commit and suite counts.

- [Run manifest and exact commands](evidence/run-manifest.json)
- [Coverage scope inventory](evidence/coverage-scope.json)
- [PlatformIO native test output](evidence/platformio-native.log)
- [C++ coverage summary](evidence/firmware-coverage-summary.txt)
- [C++ annotated lines and branch counts](evidence/firmware-annotated-coverage.txt)
- [C++ machine-readable coverage](evidence/firmware-coverage.json)
- [Python test output](evidence/python-tests.log)
- [Python coverage summary](evidence/python-coverage-summary.txt)
- [Python machine-readable coverage](evidence/python-coverage.json)
- [Python annotated HTML](evidence/python-html/index.html)

Method reference: [Clang source-based coverage documentation](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html).
Execution evidence, rather than documentation examples, supplies every result here.

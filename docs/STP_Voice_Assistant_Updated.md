# Software Test Plan

## Voice Assistant for Multimedia Controls

Team 3 | Version 1.3 | 11 November 2026

Aligned with updated SRS v1.3, updated SAD v1.3 and VoiceAssistant_Project_Complete_Guide.txt. Prepared for Dhanush S S and Team 3. This is the STP for the revised ESP32 capture and Windows recognition prototype. Scope changes remain subject to instructor review. Planned cases below are not recorded as passed.

## 1 Purpose and scope

The plan verifies five-second INMP441 audio acquisition, framed USB serial reception, WAV creation, local Faster-Whisper transcription, seven-keyword classification, action mapping, Windows key dispatch and CLI orchestration. It covers normal, boundary and relevant error conditions using unit, integration and physical system tests.

The laptop is required. Cloud transcription is excluded; cached-model offline inference is tested separately from initial model installation and online media playback. Wake word, continuous listening, on-board tinyML, physical microphone mute, BLE HID/pairing/retry, Wi-Fi provisioning, factory reset, battery listening, LED/buzzer and OTA/signing are deferred in this proposed baseline. Their original test IDs remain traceable and must not be marked passed through unrelated prototype tests.

Definitions: test case is a reproducible procedure and expected outcome; test record contains actual execution evidence; unit tests isolate one component; integration tests exercise interfaces; system tests use the real board, model and Windows host; RTM maps requirements to cases. Deferred is a scope disposition, not an Accepted status.

## 2 Team and test responsibilities

| Member and SRN | Development and preparation responsibility | Independent verification responsibility |
|---|---|---|
| Dhanush S S — PES2UG24CS154 | main.py, CLI setup, integration, scheduling, repository/report publication and joint release coordination. | Cross-test Chiranthan's mapper/controller, dispatch outcomes and diagnostics. Independently verify NF-005 module replacement. |
| G S Dheekshith — PES2UG24CS166 | Firmware, INMP441/I2S, receive_audio.py, protocol/WAV tests and audio cleanup proposals. | Cross-test Dhanush's CLI, orchestration and failure containment; independently test setup usability with Harsh coordinating. |
| Harsh Mannan — PES2UG24CS183 | Faster-Whisper and classifier tests, local model configuration, recognition dataset and accuracy evaluation; coordinate document reconciliation. | Cross-test Dheekshith's capture, framing, WAV creation and approved audio-retention behavior. |
| Chiranthan Shankar — PES2UG24CS137 | Mapper/controller, key-dispatch tests, console outcome tests and Windows media behavior records. | Cross-test Harsh's recognition, keywords, UNKNOWN behavior, accuracy and local-processing evidence. |

Owners write and run development tests; independent reviewers record requirement acceptance. All four participate in end-to-end system testing. Assignments are responsibilities, not claims about historical commits. A tester does not independently approve their own module.

## 3 Test items and environment

| Item | Required configuration and evidence |
|---|---|
| Hardware | Actual ESP32-WROOM board, INMP441, USB data cable and Windows laptop; record board identity and wiring. |
| Wiring | VDD 3V3; GND GND; BCLK GPIO26; WS GPIO25; SD GPIO33; L/R GND. Verify before powering. |
| Firmware | VoiceAssistant_ESP32.ino; record source commit, Arduino core version, board selection and upload/build log. |
| Audio | Mono 16 kHz PCM16 output; 32-bit I2S input slots; five-second payload of 160,000 bytes. |
| Serial | Actual COM port, 921600 baud, request r, AUD0 plus uint32 little-endian length, payload, END0. |
| Desktop | main.py, receive_audio.py, speech_to_text.py, command_classifier.py, command_mapper.py and media_controller.py; record actual repository paths. |
| Dependencies | Python version, pyserial, faster-whisper, PyAutoGUI and their installed versions; isolated environment. |
| Model | Faster-Whisper base, CPU, int8, beam size 1; record cached model identity and availability. |
| Player | Named Windows media application/version, initial track, playback state and volume; local media for network-isolated tests. |
| Test equipment | Timing method, audio fixtures, protocol fake/serial test double, dispatch spy and network inspection method. |

Close Arduino Serial Monitor before Python takes ownership of the COM port. Record port reset/readiness behavior. Fault-injection cases use controlled fakes where physical corruption would be unreliable. A mock result does not substitute for physical system acceptance.

## 4 Strategy and test levels

Unit tests isolate read_exact, packet validation, transcript normalization, classifier, mapper and controller dispatch arguments. Recognition unit tests may inject segment objects/model failures; they do not prove real inference. Integration tests connect adjacent modules with controlled fixtures and verify that upstream failures prevent dispatch. Physical tests use the real microphone/ESP32, actual cached model and Windows media environment.

Coverage is measured separately for the Python prototype and any retained portable C++ code. Include unexecuted production source files in the stated denominator. Record line/statement and branch metrics, exclusions and tooling. Do not average incompatible runtimes or infer requirement acceptance from code coverage. No new percentage threshold is imposed by this plan.

## 5 Entry criteria and execution order

1. Review SRS/SAD scope and deferred requirements; identify the source commit under test.
2. Confirm wiring and uploaded firmware; record environment/model versions and actual port.
3. Establish approved finite serial timeout/readiness settings, expected volume press count and known keyword limitations. Cases depending on unresolved policies remain blocked.
4. Run classifier/mapper/controller and protocol unit tests, then integration fault tests.
5. Execute real acquisition, cached-model recognition and Windows action tests.
6. Execute accuracy, timing, usability and module-replacement evaluations as applicable.
7. Record actual results, defects, independent decisions and report scope before publishing.

Do not perform unapproved irreversible device-security operations. Manual flashing uses the team's established approval process. No such operation is required merely to prepare this plan.

## 6 Expected command oracle

| Spoken keyword | Expected intent | Expected action | Expected host key |
|---|---|---|---|
| play | PLAY | media_play | playpause |
| stop | PAUSE | media_pause | playpause |
| next | NEXT | media_next_track | nexttrack |
| previous | PREVIOUS | media_previous_track | prevtrack |
| mute | MUTE | audio_mute | volumemute |
| increase | VOLUME_UP | volume_increase | volumeup |
| decrease | VOLUME_DOWN | volume_decrease | volumedown |

PLAY and PAUSE use the same toggle. Record the initial state and the actual transition; do not certify state-selecting semantics. Host mute does not establish physical microphone mute. The exact repeated volume press count is taken from the reviewed implementation before testing. Resume is not a supported keyword in this guide baseline.

## 7 Detailed functional procedures

The existing TC IDs identify revised procedures, not additional silently created requirement IDs. Every procedure below requires an execution record with actual result, tester, date and evidence; these fields remain unfilled until run.

### TC WW 02 Audio capture and stream

Reference TC-WW-02, VAMC-F-002. Owner Dheekshith; independent tester Harsh. With actual wiring and firmware, send r on the selected 921600-baud port while presenting a known test sound during capture. Record AUD0, decoded length, exactly 160,000 payload bytes and END0. Inspect WAV metadata for one channel, two-byte sample width, 16 kHz and 80,000 frames. Compare polarity/channel/sample conversion using a known fixture or inspected signed conversion. Review bounded firmware buffers; do not infer memory bounds from file length alone. Repeat a clean run to check that previous audio is not reused. Expected: complete valid audio corresponds to the current request; no text is interleaved in the packet.

### TC ASR 01 Real local transcription

Reference TC-ASR-01, VAMC-F-004. Owner Harsh; independent tester Chiranthan. Cache the base model, record configuration CPU/int8/beam1, disable recognition-network access and supply a valid known WAV. Run real transcription, preserve expected spoken content and actual joined text, and record model startup and inference times separately. Test missing/unreadable WAV and unavailable model through controlled error conditions. Expected: local inference on valid input; failure is reported without dispatch on invalid input. A mock transcript does not pass the real-inference part.

### TC ASR 02 Keyword classification

Reference TC-ASR-02, VAMC-F-005. Owner Harsh; tester Chiranthan. Execute all seven oracle keywords, uppercase/mixed-case forms and punctuation forms such as Play. and Next song. Compare exact intent with Section 6. Test unrelated words containing keyword substrings to verify word-based matching; compare the actual normalization algorithm against the SRS before choosing punctuation expectations for hyphenated phrases. Expected: documented normalization and keyword behavior. Record policy-dependent negation/multiple-keyword inputs separately rather than assuming semantic understanding.

### TC ASR 03 Unknown and failure containment

Reference TC-ASR-03, VAMC-F-006. Owner Harsh; tester Chiranthan. Supply empty, punctuation-only and unrelated transcripts with a dispatch spy attached. Confirm UNKNOWN then unknown and zero media-key calls. Inject recognition failure and verify orchestration does not execute an old action. Characterize do not play and play next as known limitation cases; a deterministic rejection rule requires a separately approved policy. Expected: unsupported input cannot dispatch; no fabricated negation/ambiguity guarantee.

### TC CMD 01 Intent mapping

Reference TC-CMD-01, VAMC-F-007. Owner Chiranthan; tester Dhanush. Feed each of the seven exact intents from Section 6 to map_command and compare the action string. Supply UNKNOWN and an invalid intent. Expected: exact oracle mapping for valid intents, unknown for unsupported input. This tests mapping independently of audio or Windows.

### TC CMD 02 Windows dispatch and observed media behavior

Reference TC-CMD-02, VAMC-F-008. Owner Chiranthan; tester Dhanush. First use a dispatch spy to verify the exact PyAutoGUI key and number of calls for each action. Then use Windows and a named player to run all seven actions with recorded initial state. Verify next/previous track changes, host volume/mute changes and playpause transitions. Test both initial playback states to expose toggle semantics. Record key dispatch and observed host effect separately. Unsupported actions and injected OS failures must not report false success. The former BLE two-second acceptance is deferred.

### TC CMD 04 Console stage outcomes

Reference TC-CMD-04, VAMC-F-010. Owner Chiranthan; tester Dhanush. Run a normal command and inspect transcript, intent, mapped action and outcome. Inject acquisition, transcription and dispatch failures one at a time with controlled fixtures; compare printed stages to actual calls. Expected: failure stage is identifiable, downstream execution stops and no success message follows a failed dispatch. No physical LED/buzzer behavior is accepted by this test.

### TC CONN 01 CLI and port setup

Reference TC-CONN-01, VAMC-F-011. Owner Dhanush; tester Dheekshith. Run without the port argument, with the actual port, with a nonexistent port and with the port owned by Serial Monitor. Expected: useful usage/error for invalid setup, correct acquisition path for a valid available port, and no unintended key on failure. Record exit behavior without inventing an unimplemented exit-code convention. This is not Wi-Fi provisioning acceptance.

### TC CONN 02 Binary protocol boundaries and recovery

Reference TC-CONN-02, VAMC-F-012. Owner Dheekshith with Dhanush integration; tester Harsh for receiver and Dheekshith for Dhanush's orchestration. Provide controlled serial responses split into single-byte and irregular chunks, including across header/length/trailer boundaries. Inject wrong AUD0, missing header, zero/short/oversized declared length, early disconnect, missing payload bytes, wrong END0 and stale data before a fresh packet. Expected: complete valid frames reconstruct; malformed/unsupported frames terminate under the approved finite timeout without unbounded allocation, valid-looking partial WAV or key dispatch. Pre-create a stale WAV and verify a failed current acquisition cannot return it as fresh input. Confirm resource release and successful later clean invocation. In-stream resynchronization is tested only if implemented and approved.

### TC DIAG 01 Failure diagnostics and disclosure

Reference TC-DIAG-01, VAMC-F-014. Owner Chiranthan; tester Dhanush. Inspect normal/error logs, command stage labels and exception messages. Expected: actionable stage information, no secrets, and no false claim that host playback occurred solely because dispatch returned. Transcripts are locally visible by design and disclosed as a baseline change; do not certify the former command-ID-only circular-log contract.

## 8 Quality and security procedures

### TC PERF 01 Timing characterization

Reference TC-PERF-01, NF-001. Owner Dheekshith, Dhanush integration; independent reviewer Harsh. Record request start, capture completion, transcription completion, dispatch time and observed media action when measurable. Separate model loading from warm inference and distinguish transport duration from capture duration where streaming overlaps. Record raw trials and measurement method. The old 50-trial p90 <= 3-second wake-to-action criterion is deferred, because capture alone takes five seconds. Do not declare a new performance pass limit without approval.

### TC ACC 01 Recognition evaluation

Reference TC-ACC-01, NF-002. Owner Harsh; tester Chiranthan. Prepare 100 documented utterances covering all seven keywords, with per-command counts declared before execution. Record expected intent, actual transcript/intent and correctness per utterance. Use SNR >= 15 dB and document the measurement method; if SNR is unknown, the original condition is unverified. Accuracy is correct intents divided by 100; the retained target is >= 90 percent. Record confusion counts and environmental conditions. Do not select successful demo clips as the evaluation dataset.

### TC UX 01 Setup usability

Reference TC-UX-01, NF-003. Owner Harsh; tester Dheekshith. Recruit five first-time users and record the exact instructions and start/end definitions. Separate dependency/model installation from ready-environment port selection and first run. Preserve actual times and difficulties. The old average <= 5-minute setup target requires review for the changed Windows environment; do not silently exclude installation to claim it passed.

### TC MAINT 01 Module replaceability

Reference TC-MAINT-01, NF-005. Coordinator Dheekshith; independent tester Dhanush. Replace one component with a test double preserving its public interface. Run the relevant integration regression and inspect the diff. Expected: no changes to other modules' internals are needed. Record the replaced module, interface, source commits and test results; static presence of multiple files alone is insufficient.

### TC SEC 01 Local processing

Reference TC-SEC-01, SR-001. Owner Harsh; tester Chiranthan. Cache model files first, use a local media file and disable recognition-network access. Capture network activity using a documented method while acquiring/transcribing/dispatching. Expected: recognition works locally and no audio/transcript is sent externally by the recognition pipeline. Model download and player network activity must be identified separately. A disconnected screenshot or successful mocked recognizer alone is insufficient.

### TC SEC 04 Audio lifecycle

Reference TC-SEC-04, SR-004. Owner Dheekshith; tester Harsh. Begin with an inventory of generated WAVs and temporary files. Run success, acquisition failure, transcription failure and interrupted-execution scenarios. Inspect files and relevant resource/buffer handling after each path. Record exact retention behavior. The guide saves recording.wav and gives no approved cleanup policy, so original no-retained-audio acceptance remains unresolved. Block final acceptance until a policy is approved and behavior meets it; local inference does not imply audio deletion.

## 9 End to end integration procedure

Coordinator Dhanush; all four participate, with Dheekshith independently checking orchestration. Upload the identified firmware, close Serial Monitor, start the desktop application with the actual port and speak one controlled command during each recording window. Execute each of the seven oracle commands in separate runs. Preserve request/packet details, WAV metadata, transcript, intent, mapped action, dispatch event and visible host result. Repeat with unsupported input, disconnected serial and recognition/dispatch failures. Expected: valid stages compose correctly; failed stages prevent unintended execution. Use a local player for recognition-offline evidence. No wake-word/BLE/on-board recognition claim is inferred from this system path.

## 10 Complete requirement traceability matrix

All rows refer to the revised SRS v1.3. Scope dispositions below do not overwrite historical test records or RTM A/F values.

| Requirement | Test reference and prototype disposition | Owner and independent tester |
|---|---|---|
| VAMC-F-001 | TC-WW-01 deferred wake word | Dheekshith / Harsh |
| VAMC-F-002 | TC-WW-02 revised capture/stream | Dheekshith / Harsh |
| VAMC-F-003 | TC-WW-03 deferred physical mute | Dheekshith / Harsh |
| VAMC-F-004 | TC-ASR-01 revised laptop local transcription | Harsh / Chiranthan |
| VAMC-F-005 | TC-ASR-02 revised keyword classifier | Harsh / Chiranthan |
| VAMC-F-006 | TC-ASR-03 revised unsupported/no-dispatch | Harsh / Chiranthan |
| VAMC-F-007 | TC-CMD-01 revised seven mappings | Chiranthan / Dhanush |
| VAMC-F-008 | TC-CMD-02 revised Windows dispatch | Chiranthan / Dhanush |
| VAMC-F-009 | TC-CMD-03 deferred BLE retry | Chiranthan / Dhanush |
| VAMC-F-010 | TC-CMD-04 revised console feedback | Chiranthan / Dhanush |
| VAMC-F-011 | TC-CONN-01 revised CLI/port setup | Dhanush / Dheekshith |
| VAMC-F-012 | TC-CONN-02 revised protocol/WAV | Dheekshith with Dhanush / Harsh receiver; Dheekshith orchestration |
| VAMC-F-013 | TC-CONN-03 deferred physical reset | Dhanush / Dheekshith |
| VAMC-F-014 | TC-DIAG-01 revised console diagnostics | Chiranthan / Dhanush |
| VAMC-F-015 | TC-DIAG-02 deferred OTA | Chiranthan / Dhanush |
| VAMC-NF-001 | TC-PERF-01 characterization; old latency criterion deferred | Dheekshith / Harsh |
| VAMC-NF-002 | TC-ACC-01 retained accuracy target; actual evaluation required | Harsh / Chiranthan |
| VAMC-NF-003 | TC-UX-01 setup interpretation pending review | Harsh / Dheekshith |
| VAMC-NF-004 | TC-POWER-01 deferred battery idle listening | Dheekshith / Harsh |
| VAMC-NF-005 | TC-MAINT-01 active module replacement | Dheekshith / Dhanush |
| VAMC-SR-001 | TC-SEC-01 revised local recognition/privacy | Harsh / Chiranthan |
| VAMC-SR-002 | TC-SEC-02 deferred BLE authentication | Dhanush / Dheekshith |
| VAMC-SR-003 | TC-SEC-03 deferred Wi-Fi credential storage | Dhanush / Dheekshith |
| VAMC-SR-004 | TC-SEC-04 unresolved audio-retention policy | Dheekshith / Harsh |
| VAMC-SR-005 | TC-SEC-05 deferred OTA signatures | Chiranthan / Dhanush |

Deferred cases remain accessible in the historical STP. If their capabilities return to scope, use the original criteria and re-review their procedures. No deferred case is counted as a passed active case.

## 11 Test records and defect management

Each executed case records case ID and revision, linked requirement, source commit, firmware/model/dependency identity, prerequisites, input, steps, expected result, actual result, tester, date/time, artifacts, defects and independent decision. Leave actual result, execution date and signature empty until run. Status codes are N not started, P in progress, T ready for test, A independently accepted, F independently failed and B blocked. Record scope disposition separately so deferred is never confused with A.

For each defect capture reproducible steps, expected/actual behavior, affected commit/environment, severity, owner and retest evidence. Wrong-action dispatch, stale-audio execution and externally transmitted speech violate active behavior/privacy expectations and block the corresponding acceptance. Fixes require targeted retesting and affected integration regression. Independent reviewers verify closure.

## 12 Exit criteria and suspension

Active requirements have attributable actual results; all seven mappings/keys are checked; protocol and upstream failures prevent dispatch; local inference is tested with cached model; measured accuracy conditions are documented; unresolved retention and changed scope decisions are reviewed; significant open defects and deferred work are disclosed. Instructor approval remains separate from team testing.

Suspend physical execution when wiring/power is unsafe, the test build is unidentified, framing corruption cannot be contained, or dispatch causes unintended actions. Resume after the owner resolves the cause and the independent tester checks the affected setup. Do not fabricate replacement results for unavailable hardware or Windows access.

## 13 Reports and publication

Produce Unit Test Report, System Validation Report, Code Coverage Report and Branch Coverage Report under the repository docs folder. Attach raw logs, machine-readable coverage, test inventory, source hashes, environment details and measured exclusions. Dhanush coordinates publication; each module owner supplies evidence; independent testers validate acceptance entries.

The 7 November bundle records 93 C++ cases and 10 Python methods with its own explicit source scope. It is historical evidence, not execution of all procedures in this new STP. The supplied demo is limited visual evidence of the ESP32/serial and laptop path. Project-lead issue closures are reported status updates, not new measurements. Use actual execution dates rather than backdating to the course window of 2-6 November.

## 14 Risks and pending decisions

| Risk or decision | Responsible member | Plan impact |
|---|---|---|
| Serial readiness/timeout/recovery values not fixed | Dheekshith and Dhanush | Block timing-dependent boundary acceptance until values are documented. |
| Negation and multi-keyword policy | Harsh with team review | Characterize current limitations; no invented rejection oracle. |
| Volume repetitions and toggle semantics | Chiranthan | Record implementation count and initial host state before tests. |
| Raw WAV retention/deletion policy | Dheekshith with team review | SR-004 remains unresolved; collect success/error lifecycle evidence. |
| Revised latency/setup expectations | Dhanush and SRS owners | Measure first; obtain approval before declaring replacement limits. |
| Windows/device/model access | All four | Mark unavailable tests honestly and schedule independent execution. |
| Instructor scope reconciliation | Dhanush and Harsh | Revised SRS/SAD/STP and backlog require coordinated approval. |

## 15 Revision and approvals

Version 1.3 dated 11 November 2026 aligns the test plan with the supplied guide and revised SRS, preserves all 25 requirement/test references, documents deferred scope and allocates all four members' work. Harsh coordinates technical reconciliation; Dhanush coordinates the submission. All members review their module procedures. Actual execution results, team acceptance and instructor approval must be entered by the responsible people; none are prefilled here.

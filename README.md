# Voice Assistant for Multimedia Controls

Team 3's revised prototype uses an INMP441 microphone and ESP32 for audio acquisition, USB serial for transfer, and a Windows laptop for local speech recognition and media-key execution.

## Current documentation baseline

Version 1.3, dated 11 November 2026, follows the supplied project guide. The team lead requested publication; instructor review of the scope changes remains pending.

| Document | PDF | Editable Word | Markdown |
|---|---|---|---|
| Software Requirements Specification | [SRS](SRS_Voice_Assistant_Updated.pdf) | [Word](docs/SRS_Voice_Assistant_Updated.docx) | [Source](docs/SRS_Voice_Assistant_Updated.md) |
| Software Architecture Document | [SAD](3_SE_SAD.pdf) | [Word](docs/SAD_Voice_Assistant_Updated.docx) | [Source](docs/SAD_Voice_Assistant_Updated.md) |
| Software Test Plan | [STP](3_SE_STP.pdf) | [Word](docs/STP_Voice_Assistant_Updated.docx) | [Source](docs/STP_Voice_Assistant_Updated.md) |

These replace the older published specifications at the existing PDF paths. Previous versions remain in Git history. See the documents' change registers and [deviations](docs/DEVIATIONS.md).

## Guide architecture

INMP441 -> I2S -> ESP32 -> USB serial -> WAV -> local Faster-Whisper -> keyword classification -> action mapping -> PyAutoGUI -> Windows media keys.

The guide specifies one five-second recording per invocation, 16 kHz mono PCM16, 921600-baud serial and AUD0/length/payload/END0 framing. Recognition runs on the laptop. A cached model permits local inference; installation and initial model preparation may need internet. Cloud speech recognition is not part of this design.

Guide keywords: **play, stop, next, previous, mute, increase, decrease**. Stop maps to PAUSE; play and stop both use the playpause toggle, so a requested playback state is not guaranteed. Mute affects host audio, not the microphone. Wake word, on-board recognition and BLE HID are deferred in this revised baseline.

## Team and work division

| Member | SRN | Assigned part | Independent cross-tester |
|---|---|---|---|
| Dhanush S S | PES2UG24CS154 | Integration, main.py, CLI setup and publication | G S Dheekshith |
| G S Dheekshith ([@dheekshithgs](https://github.com/dheekshithgs)) | PES2UG24CS166 | INMP441/I2S, firmware, serial reception and WAV creation | Harsh Mannan |
| Harsh Mannan ([@Harshaltt](https://github.com/Harshaltt)) | PES2UG24CS183 | Local recognition, classifier, accuracy and document reconciliation | Chiranthan Shankar |
| Chiranthan Shankar ([@ChiruDevv](https://github.com/ChiruDevv)) | PES2UG24CS137 | Mapping, media execution and diagnostics | Dhanush S S |

Assignments describe responsibility, not retrospective authorship. All four participate in system validation.

## Implementation and evidence

[Existing laptop prototype](companion/laptop_prototype/README.md) preserves Dheekshith's contribution and subsequent review fixes. Its actual protocol, command vocabulary and dispatch adapter differ from the guide; follow its own README to run that code. The new specifications do not certify that the existing implementation already conforms to every revised contract. Firmware and companion source are unchanged by this documentation replacement.

- [Published prototype video and observations](docs/demos/review-1/README.md)
- [Test reports and measured evidence](docs/test-reports/2026-11-07/README.md)
- [Latest testing addendum](docs/test-reports/2026-11-07/UPDATE_2026-11-11.md)

Historical test results apply to their recorded source baselines and exclusions. The STP defines revised tests and all 25 requirement dispositions; planned cases and deferred scope are not passed results.

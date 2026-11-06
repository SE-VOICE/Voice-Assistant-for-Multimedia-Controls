# Voice Assistant for Multimedia Controls

A hardware-based, voice-controlled multimedia remote built on **ESP32**. Say a wake word, speak a command, and control playback on a paired host device (phone, PC, TV, or Bluetooth speaker) — no touching a screen or remote required.

**Team 3**

Experimental contribution by [@dheekshithgs](https://github.com/dheekshithgs):
[USB/laptop voice-assistant prototype](companion/laptop_prototype/README.md).
This optional prototype is separate from the target ESP32 recognition/BLE path;
see its documented test results and limitations before running it.

| SRN | Name | Role |
|---|---|---|
| PES2UG24CS154 | Dhanush S S | Project Manager & Integration Lead |
| PES2UG24CS166 | G S Dheekshith | Audio Module Lead |
| PES2UG24CS183 | Harsh Mannan | Recognition Module Lead |
| PES2UG24CS137 | Chiranthan Shankar ([@ChiruDevv](https://github.com/ChiruDevv)) | Output & Maintenance Module Lead |

---

## Overview

The assistant listens continuously for a wake word on-device, then streams the captured command to a cloud speech-to-text service for recognition. Recognized commands are translated into Bluetooth Low Energy (BLE) HID media-key events and sent to a paired host, which executes the action.

**Supported commands:** volume up, volume down, pause, resume, play, next, previous.

```
 Wake Word ──▶ Capture Audio ──▶ Cloud ASR ──▶ Command Mapping ──▶ BLE HID ──▶ Host Media Player
 (on-device)                    (speech-to-text)                 (transmit)   (executes action)
```

## Features

- 🎙️ On-device wake-word detection (no audio leaves the device until triggered)
- ☁️ Cloud-based speech recognition for accurate command transcription
- 🔊 7 core multimedia commands via Bluetooth HID
- 🔗 Guided Wi-Fi provisioning (captive portal) and Bluetooth pairing
- 💡 LED/buzzer feedback for listening, success, and error states
- 🔁 Retry/backoff logic for unreliable BLE links
- 🛠️ OTA firmware updates with signature verification
- 🧪 Optional Python + MediaPipe companion module for PC-side testing/gesture confirmation

## Hardware & Tech Stack

| Category | Details |
|---|---|
| Microcontroller | ESP32 (e.g., ESP32-WROOM) |
| Microphone | I2S / analog MEMS mic |
| Connectivity | 2.4GHz Wi-Fi, Bluetooth 4.2+ / BLE |
| Firmware language | C / C++ (Arduino / ESP-IDF) |
| Speech recognition | Cloud ASR API (e.g., Google Cloud Speech-to-Text) over HTTPS |
| Companion tooling | Python, MediaPipe |
| Power | USB or Li-ion battery (e.g., 2000mAh) |

## Repository Structure

```
.
├── firmware/           # ESP32 firmware (wake-word, ASR client, BLE HID, OTA)
├── companion/          # Optional Python + MediaPipe testing/companion module
├── docs/
│   ├── SRS_Voice_Assistant_Multimedia_Controls.docx
│   ├── SRS_Voice_Assistant_Multimedia_Controls.md
│   └── images/          # UML use-case diagrams
└── README.md
```

> Adjust this tree to match your actual folder layout as the codebase grows.

## Getting Started

### Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) or the [Arduino IDE](https://www.arduino.cc/en/software) with ESP32 board support
- An ESP32 dev board + I2S/analog MEMS microphone
- A cloud ASR account/API key (e.g., Google Cloud Speech-to-Text)
- Python 3.9+ (only if using the companion module)

### Setup

1. Clone the repo:
   ```bash
   git clone <repo-url>
   cd <repo-name>
   ```
2. Flash the firmware in `firmware/` to your ESP32 (see in-folder instructions).
3. On first boot, connect to the device's Wi-Fi captive portal to provision your home Wi-Fi.
4. Pair the device with your host over Bluetooth (phone/PC/TV that supports BLE HID media keys).
5. Say the wake word, then speak a command (e.g., "next", "pause").

### Optional: companion module

```bash
cd companion
pip install -r requirements.txt
python main.py
```

## Documentation

Full requirements, architecture, and UML diagrams live in [`docs/SRS_Voice_Assistant_Multimedia_Controls.md`](docs/SRS_Voice_Assistant_Multimedia_Controls.md), including:

- Functional & non-functional requirements (`VAMC-F-###` / `VAMC-NF-###`)
- Security requirements (`VAMC-SR-###`)
- UML use-case diagrams with interpretation notes
- Requirements Traceability Matrix (RTM)
- Team work division & cross-testing plan
- Assumptions, out-of-scope items, and a final consistency checklist

## Testing & Cross-Testing

Each module is developed by one team member and cross-tested by another (no one tests their own code) — see Section 9 of the SRS for the full rotation and status-code conventions (`N` / `P` / `T` / `A` / `F` / `B`).

## License

_Add a license (e.g., MIT) before making this repository public, if applicable._

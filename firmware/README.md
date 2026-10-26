# Team 3 firmware foundation (DR-01)

This branch is scaffolding, not a functioning voice assistant.
Approved framework: PlatformIO + Arduino-ESP32. Board: pending.

## Run the computer tests

Install Python 3 and PlatformIO Core in a virtual environment, then:

```sh
cd firmware
pio test -e native
```

Native tests need a C++ compiler (Xcode command-line tools on macOS,
build-essential on Linux, or a supported GCC toolchain on Windows).
PlatformIO documentation: https://docs.platformio.org/en/latest/platforms/native.html
Unity documentation: https://docs.platformio.org/en/latest/advanced/unit-testing/frameworks/unity.html

DR-01 has no STP test ID. The three DR-01 smoke tests check portable interface
compilation and fake feedback/log substitution; they do not accept any feature.
No hardware, real log buffer, BLE acknowledgement, cloud call, or Wi-Fi connection
is exercised. main.cpp is an inert Arduino entry point excluded from native tests.

## Separate ownership

| Owner | Future implementation folders | Next story |
|---|---|---|
| Dhanush | lib/connectivity, corresponding src/hal adapters | VAMC-F-011 |
| Dheekshith | lib/wake_word, audio HAL; CI separately | PRC-01, VAMC-F-001 |
| Harsh | lib/asr_client | VAMC-F-004 |
| Chiranthan | lib/command_map, lib/ble_hid, lib/feedback, lib/diagnostics_ota | VAMC-F-007 |

The unfinished module APIs use forward declarations rather than guessing
provider/engine/pairing contracts. All implementation work stays on one branch and
PR per story. Use public headers; keep hardware code in src/hal. Team members
raise their own PRs; cross-testers verify, and a human owner reviews and merges.

## Before the ESP32 build

Select the physical board and its PlatformIO board ID with the team. Then add
an esp32dev environment using a pinned Espressif32 platform and framework=arduino.
No board, pins, provisioning scheme, or credentials are guessed here.
`pio run -e esp32dev` is expected to report an unknown environment for now.
Do not upload or flash a board without separate approval.

CI is PRC-01 (#14), owned by Dheekshith, and is not installed by this story.
Both native tests and the approved ESP32 build must later be wired into CI.
The remaining GitHub-process criteria of DR-01 are not marked complete.

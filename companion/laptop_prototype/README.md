# Experimental USB/laptop companion

Original prototype: G S Dheekshith ([@dheekshithgs](https://github.com/dheekshithgs)).
The four original commits are preserved. Subsequent review fixes are authored
separately by Dhanush with AI assistance, not attributed to Dheekshith.

The team lead approved this optional experiment on 7 November 2026. It does not
replace the approved ESP32-WROOM offline recognizer, wake word or BLE HID path.
No firmware story is declared accepted by this companion's merge.

Flow: ESP32 serial audio -> laptop local Faster-Whisper -> strict phrase match
-> opt-in Windows foreground-app media message. No speech API is called.
Python 3.10+ is recommended. Recognition needs a compatible pre-downloaded
Faster-Whisper model directory; runtime loading uses local_files_only=True.
Installing packages/model preparation can require internet; a model is not
included or automatically downloaded. No accuracy benchmark has been run.

## Safe text demonstration and tests

From this directory (no third-party packages needed for text-only use):

```
python3 main.py --text "pause"
python3 -m unittest discover -s tests -v
```

All seven exact phrases are supported: play, pause, resume, next, previous,
volume up, volume down. The code contains the explicit accepted variants.
Unknown, partial and multi-command phrases are rejected. Mute and stop are
not part of this experiment's approved command vocabulary.

## Optional audio setup

```
python3 -m venv .venv
# Activate the environment for your operating system, then:
python -m pip install -r requirements.txt
python main.py --port COM3 --model C:/models/faster-whisper-base
```

That command captures/transcribes but defaults to dry run. Audio is stored only
in an OS temporary directory and removed when the recognition block exits,
including exceptions. Forced termination/power loss can leave temporary data;
this is not secure erasure or proof of firmware SR-004 compliance. Transcripts
are not printed. Python/model memory cannot be guaranteed securely zeroed.

ESP32 firmware must already implement the receiver protocol: 921600 baud,
request byte r, AUD0 + little-endian uint32 length 160000, then 160000 bytes of
16 kHz signed PCM16 mono audio, followed by END0. The receiver requires exactly
five seconds. Compatible ESP32 microphone firmware is NOT provided here and
was not tested; connecting an arbitrary ESP32 is insufficient. There is no
wake-word trigger in this prototype. Serial reads time out after 60 seconds
without data, not a global session deadline.

## Explicit Windows dispatch

```
python main.py --text "pause" --execute
```

Only --execute sends a real action. Windows sends discrete PLAY (46), PAUSE
(47), NEXT (11), PREVIOUS (12), VOLUME_UP (10), VOLUME_DOWN (9) WM_APPCOMMAND
messages to the foreground window. RESUME uses PLAY. No play/pause toggle
fallback is used. The selected player must support these messages. A nonzero
application acknowledgement is not proof playback changed; inspect the player.
Live execution fails explicitly on macOS/Linux. Do not claim Mac media support.
Reference: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-appcommand

## Verification limits

Portable unit tests and syntax checks cover command coverage, ambiguity,
dispatch decisions and temporary-file cleanup with fakes. No real microphone,
serial audio stream, model inference, Windows media action, BLE or end-to-end
latency/accuracy was verified during integration. Dependencies are constrained
by version range, not a reproducible lockfile. Independent cross-testing is
still required before claiming hardware or feature acceptance.

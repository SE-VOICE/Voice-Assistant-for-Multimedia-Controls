# Review 1 prototype demo (VID-01)

[Watch or download the demo](esp32-laptop-prototype.mp4)

Submitted by project lead Dhanush S S on 11 November 2026. The filename and
on-screen Windows date indicate 2 November 2026; that is source-provided dating,
not an independently verified capture timestamp. Duration: approximately
51.44 seconds. Original: 1920 x 1080, 30 fps, H.264 with AAC audio.

The repository copy retains the full timeline and audio stream, re-encoded to
reduce download size. No padding, synthetic demonstrations or altered results
were added. The original file remains unchanged on the submitter's computer.

## Visual evidence reviewed

| Approximate time | Visible observation |
|---|---|
| 00:00 | Arduino IDE selects ESP32 Dev Module on COM3. Upload output reports written-data hash verified and reset. I2S capture code is visible. |
| 00:20 | Python terminal reports Faster-Whisper loading, PLAY classification and media_play execution. Another run requests a five-second recording from ESP32 over COM3. |
| 00:30 | Terminal reports NEXT and media_next_track. YouTube changes to the next playlist entry. |
| 00:50 | Terminal reports MUTE and audio_mute execution, then starts another recording request. |

Review method: video metadata and sampled visual frames. Spoken audio was not
independently transcribed or verified. Terminal success messages are recorded
as observations, not independent proof of every requested media action.

## Demonstrated architecture and limits

This is evidence of an **ESP32/serial plus laptop recognition prototype**.
The visible program runs Faster-Whisper on the Windows laptop and dispatches
media actions there. It does not establish a recognizer running on ESP32-WROOM,
a BLE HID transport, wake-word detection, all seven required commands, latency
or accuracy thresholds, battery life, secure pairing, credential protection,
factory reset or signed OTA acceptance.

MUTE here is a host media action; it is not proof of the physical microphone
mute requirement. A saved recording.wav path is visible: the video does not
establish the no-retained-audio requirement or demonstrate subsequent cleanup.
The physical microphone/board wiring is not shown in the reviewed frames.

The approved target remains wake word -> audio capture -> offline WROOM command
recognition -> command mapping -> Bluetooth HID -> connected device. This demo
adds progress evidence without changing that target or any requirement ID.

## Completion record

The earlier statement that no video had been supplied is now superseded.
The clip is about 51 seconds, rather than exactly 60. Team/course review should
confirm whether this short prototype demonstration satisfies VID-01's one-minute
working-product submission requirement. Independent acceptance remains pending;
no historical test report or RTM acceptance result is changed by this upload.

## Source integrity

Original filename: WhatsApp Video 2026-11-02 at 22.41.52.mp4

Original SHA-256: `292e2be5f6552a781079be6f661c02a6e85faa720901844c330da005253c959e`

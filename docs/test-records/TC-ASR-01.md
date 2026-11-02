# TC-ASR-01 — stream audio to cloud ASR over HTTPS

Requirement: VAMC-F-004
Owner: Harsh Mannan
Independent tester: Chiranthan Shankar
Status: T (ready for native cross-test; not accepted)

## Part A — native (no hardware, no cloud)

1. In `firmware`, run `pio test -e native -f test_VAMC_F_004`.
2. Confirm the request matches SAD 4.3 Interface 1 (POST to
   `speech.googleapis.com/v1/speech:recognize`, LINEAR16, 16 kHz, base64 audio,
   API key in the `X-Goog-Api-Key` header), that a 5 s capture is streamed in
   writes of at most 512 bytes, and that the transcript and confidence are read
   from plain, chunked and fragmented responses.
3. Confirm 401/403, 429 and 5xx map to AUTH_FAILED, RATE_LIMITED and
   SERVICE_UNAVAILABLE, and that timeouts, dropped connections, malformed and
   oversized responses all fail without a transcript.

## Part B — device and cloud (blocked)

Blocked on the board selection, the ESP32 TLS socket adapter in `src/hal`,
Wi-Fi provisioning, audio capture, the final ASR provider and API key
provisioning (SAD 4.6). When unblocked: speak each supported command over
normal Wi-Fi, record `AsrResult::latency_ms` for each, and verify the
transcript is returned with a round trip under 3 s.

Expected result from the STP: Audio is sent over HTTPS and transcription round-trip is < 3 s under normal Wi-Fi.

Actual result: __________
Test date: __________
Tester: __________
Board, host, Wi-Fi, ASR provider and firmware version: __________
Evidence link: __________

Passing native tests does not fill in the actual result.

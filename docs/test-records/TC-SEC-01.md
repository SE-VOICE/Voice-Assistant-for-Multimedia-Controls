# TC-SEC-01 — TLS 1.2+ for all ASR traffic

Requirement: VAMC-SR-001
Owner: Harsh Mannan
Independent tester: Chiranthan Shankar (reviewed by all members, SRS 9)
Status: T (ready for native cross-test; not accepted)

## Part A — native (no hardware)

1. In `firmware`, run `pio test -e native -f test_VAMC_SR_001`.
2. Confirm that TLS 1.0/1.1/unknown sessions, unverified server certificates,
   a missing CA certificate and any non-`https://` URL are all refused before a
   byte is written, and that TLS 1.2 and 1.3 verified sessions open.
3. Confirm the channel requests TLS 1.2 as the minimum version and passes the
   CA certificate to the TLS stack before the handshake.

## Part B — network capture (blocked)

Blocked on the board selection, the ESP32 TLS socket adapter in `src/hal`, the
ASR provider choice and Wi-Fi provisioning. When unblocked:

1. Capture the device's traffic during an ASR request (e.g. Wireshark on the
   access point or a mirrored port).
2. Verify the ClientHello offers no version below TLS 1.2 and the ServerHello
   selects TLS 1.2 or 1.3.
3. Point the device at a server presenting an untrusted certificate (e.g. a
   mitmproxy instance) and verify no request or audio is sent.

Expected result from the STP: All ASR traffic uses TLS 1.2+.

Actual result: __________
Test date: __________
Tester: __________
Board, host, and firmware version: __________
Evidence link: __________

Passing native tests does not fill in the actual result.

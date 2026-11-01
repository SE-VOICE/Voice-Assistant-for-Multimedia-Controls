# Hardware adapters

Board-dependent I2S, BLE, Wi-Fi, GPIO and storage adapters belong here.
No board, pins, pairing policy or credential storage scheme has been selected.
Portable module headers must not include Arduino or ESP32 headers.

## Signed OTA adapter

`esp32_signed_ota_backend.*` uses Arduino-ESP32 `Update` only when compiled for
ESP32. It refuses to stage an image unless signature support is enabled and a
trusted RSA public key is supplied. The application build must define
`UPDATE_SIGN`, `VAMC_OTA_PUBLIC_KEY` (the public-key byte array symbol), and
`VAMC_OTA_PUBLIC_KEY_LEN` (its byte count). `UpdaterRSAVerifier` checks the
image when `Update.end()` finalizes it. The verifier stays alive across the
update because the Arduino updater retains its pointer.

The project has not selected its ESP32 board, signing-key owner, public key, or
firmware hosting and transport configuration. Those must be supplied by the
team before this adapter can be built and used on a device. Keep the private
signing key out of the repository and device firmware. Until the trusted public
key is configured, `enable_signature_verification()` returns false and OTA is
blocked. Native tests exercise the fail-closed policy and update orchestration;
they do not perform cryptographic verification or hardware OTA.

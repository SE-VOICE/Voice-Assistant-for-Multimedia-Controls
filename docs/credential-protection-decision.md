# SR-003 approval and verification boundary

Dhanush explicitly approved entering an unlock passphrase on every boot, with
the derived key retained only in RAM, on 7 November 2026. This replaces the
previous unimplemented persistence choice. No irreversible eFuse, secure-boot
or flash-encryption configuration is approved or required by this change.

Implementation: AES-256-GCM authenticated encryption, PBKDF2-HMAC-SHA256 with
600000 iterations and a random 16-byte salt; a fresh 12-byte nonce for every
save; 16-byte authentication tag. VA01 is a fixed 146-byte record, containing
no passphrase or key. The native backend uses OpenSSL; ESP32 uses bundled
mbedTLS. The NVS adapter stores only the encrypted record under vamc_sec/wifi.
No bespoke encryption algorithm is implemented.

The passphrase must be provided through a local serial input that does not echo
or log it. Use a long unique passphrase; a flash dump permits offline guessing.
Loss of the passphrase requires factory reset/reprovisioning; no recovery key
is embedded. An unlocked running device can expose RAM secrets to an attacker.
The caller must clear its input buffer and loaded credentials after Wi-Fi setup.
Vault buffers and key/salt are wiped on failure/lock/destruction. Wi-Fi adapter
must disable SDK persistence BEFORE calling WiFi.begin, otherwise the SDK can
store another plaintext copy. Do not claim flash privacy without device inspection.

Radio entropy must be initialized before new salt/nonce generation on ESP32.
PBKDF2 boot latency needs target measurement; 600000 is a versioned software
parameter, not an achieved timing result or changed SRS acceptance threshold.
Native tests use synthetic credentials only. Host OpenSSL tests do not prove
the ESP32 mbedTLS path, flash contents, residual SDK storage, or physical reset.
Independent TC-SEC-03 remains T/acceptance pending until board inspection.

Host setup: install OpenSSL development headers (macOS: Homebrew openssl@3;
Linux: libssl-dev). PlatformIO native build links libcrypto. ESP32 uses its
framework libraries. Never put real credentials/passphrases in source or tests.

`firmware/examples/VaultBoot/VaultBoot.ino` demonstrates non-echoing, bounded
serial input and boot unlock. It initializes Wi-Fi radio entropy with SDK
persistence disabled, then opens NVS. It deliberately does not provide a portal,
connect to an access point or claim full F-011 integration. Copy the four files
listed at the top of the sketch beside it for an Arduino IDE compile. The real
example (including vault method references) compiled for generic ESP32 using
Arduino-ESP32 3.3.12: 882000 bytes flash and 45888 bytes static RAM. These are
compile sizes, not runtime heap, unlock timing or hardware security results.

# TC-SEC-03 — protected Wi-Fi credentials

Requirement: VAMC-SR-003. Author: Dhanush S S. Independent tester: G S
Dheekshith. Status: T (software ready for independent/device testing, not A).

Approved design: boot-time passphrase; derived key only in RAM. See
`../credential-protection-decision.md`. Never attach real credentials or raw
flash dumps containing personal information to a public repository.

## Procedure

1. Use a dedicated test ESP32-WROOM and synthetic network credentials. Record
   board, firmware commit, framework version and flash layout.
2. Initialize radio entropy, disable SDK Wi-Fi persistence before connection,
   and enter a long unique passphrase through local non-echoing input. Clear
   the input after unlock. Provision synthetic credentials using the vault.
3. Inspect the stored `vamc_sec/wifi` record and the rest of flash. Expected:
   encrypted record only; no plaintext synthetic SSID/password, passphrase or
   derived key. Check legacy SDK namespaces too: a new vault does not erase
   credentials previously stored by other firmware.
4. Reboot. Expected: credentials unavailable until the correct passphrase is
   entered. Wrong passphrase must fail without returning credentials.
5. Modify one ciphertext/tag byte on the test device. Expected: authentication
   fails, output buffers remain cleared, no connection using corrupt data.
6. Restore/provision and save again. Expected: a fresh nonce, successful reboot
   unlock and Wi-Fi connection. Record unlock duration and watchdog behavior.
7. Lock/reset. Expected: key inaccessible through the vault; reset removes the
   stored record. This does not establish forensic flash erasure.

## Actual independent result

Tester:
Date:
Firmware commit:
Board / framework:
Flash inspection evidence:
Unlock timing / watchdog observations:
Pass/fail and limitations:

## Author's software evidence — 7 November 2026

`pio test -d firmware -e native`: 81/81 passed, including six real OpenSSL
vault tests (round trip/reboot, wrong passphrase, tampering, fresh nonce/erase,
storage errors, malformed record). Arduino-ESP32 3.3.12 generic ESP32 compile
smoke check passed. No device was flashed; these results do not fill the
independent result above or establish plaintext absence in actual flash.

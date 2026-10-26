# TC-CONN-01 — Guided Wi-Fi provisioning

- Requirement: VAMC-F-011
- Tester: G S Dheekshith (cross-tester)
- Date: ____________________
- Target board / firmware revision: ____________________

## Procedure

1. Start with the selected ESP32 at first boot or after a factory reset.
2. Join the setup network and open the captive portal page.
3. Enter a valid home Wi-Fi SSID and password and submit.
4. Observe the device joining the home network and the setup page/portal ending its setup session.
5. Record elapsed time from the start of guided setup to successful home-network connection.
6. Repeat with an invalid password and confirm setup reports/retains a retry path without claiming success.

## Expected result

The guided setup connects a new device to the home Wi-Fi within five minutes. Invalid credentials do not produce a success indication. Credential persistence must use the team-approved SR-003 protection scheme; no plaintext flash storage is acceptable.

## Actual result

Not run. ESP32 target, captive portal adapter, and SR-003 credential-protection decision are pending.

## Evidence / notes

__________________________________________________________________

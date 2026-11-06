# TC-CONN-02 — Bluetooth discovery and pairing

- Requirement: VAMC-F-012
- Tester: G S Dheekshith (cross-tester)
- Date: ____________________
- ESP32 board / BLE stack: ____________________
- Host device and OS: ____________________

## Procedure

1. Power on the device and start its BLE setup/discoverable mode.
2. On the host, open Bluetooth settings and scan for nearby devices.
3. Confirm the assistant appears in the list using the device name configured by the team.
4. Select it and confirm the host requests the displayed passkey.
5. Complete pairing and verify the device is shown as paired on the host.
6. Repeat with a pairing attempt that does not use passkey authentication; confirm it is rejected and any untrusted bond is removed.

## Expected result

The device appears in the host's Bluetooth list and pairs successfully. Authenticated pairing uses the setup-page passkey policy; Just Works must not be accepted as authenticated pairing.

## Actual result

Not run. Board, BLE stack, advertising adapter, setup page and host pairing are not available in this portable implementation.

## Evidence / notes

__________________________________________________________________

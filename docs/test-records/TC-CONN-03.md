# TC-CONN-03 — factory reset

Requirement: VAMC-F-013
Owner: Dhanush S S
Independent tester: G S Dheekshith
Status: T (ready for hardware test; not accepted)

## Procedure

1. On the selected ESP32 board, complete Wi-Fi provisioning and BLE pairing.
2. Verify the device can use the saved settings.
3. Perform the documented long press of the physical reset button.
4. Inspect stored Wi-Fi and BLE settings, then observe whether provisioning restarts.

Expected result from the STP: Stored Wi-Fi/BLE data is cleared and provisioning restarts.

Actual result: __________
Test date: __________
Tester: __________
Board and firmware version: __________
Evidence link: __________

The button threshold and storage adapters are pending board and credential-store decisions. A passing native fake-storage test does not fill in the actual result.

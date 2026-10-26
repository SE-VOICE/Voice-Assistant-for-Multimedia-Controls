# TC-SEC-02 — authenticated BLE pairing

Requirement: VAMC-SR-002
Owner: Dhanush S S
Independent tester: G S Dheekshith
Status: T (ready for hardware test; not accepted)

## Procedure

1. On a supported host, open the device setup page and start BLE pairing.
2. Confirm that the page displays the passkey and the host requests that passkey.
3. Enter the passkey and verify that the host pairs.
4. Repeat with an incorrect passkey or a Just Works pairing attempt and verify that it is not accepted as authenticated.

Expected result from the STP: Passkey/numeric confirmation is required rather than Just Works.

Actual result: __________
Test date: __________
Tester: __________
Board, host, and firmware version: __________
Evidence link: __________

The setup page, passkey source, BLE adapter and host test are pending. Passing native fake-adapter tests does not fill in the actual result.

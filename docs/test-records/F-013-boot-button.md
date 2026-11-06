# F-013 BOOT button implementation

Dhanush approved BOOT/GPIO0 and subsequently a five-second continuous hold on
7 November 2026. `BootResetButton` defaults to the approved 5000 ms; an explicit
zero disables it. Tests cover the product boundary in addition to synthetic
short durations. It calls the existing factory-reset coordinator after a continuous
hold, once per press, and requires release after boot. Short/released presses
do not accumulate. Millisecond-counter rollover is tested.

GPIO0 is a strapping pin: do not hold BOOT during power-on/reset. This adapter
does not implement physical flash/BLE bond erasure or claim factory reset works.
The storage adapter must report actual erasure completion before provisioning
restarts; asynchronous BLE bond deletion must not be reported as already done.

Before device acceptance: integrate real Wi-Fi vault/SDK and BLE
bond removal; compile the full selected firmware; test short press, long press,
boot-held button, reset failure and reboot persistence with Dheekshith.

Independent tester/date:
Approved hold duration: 5 seconds (Dhanush, 7 November 2026)
Firmware/board:
Actual results:
RTM acceptance:

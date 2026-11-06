# F-013 BOOT button implementation

Dhanush approved BOOT/GPIO0 on 7 November 2026, with the long-press duration to
be agreed before device testing. `BootResetButton` therefore requires an explicit
duration; zero disables it. The synthetic unit-test duration is not a product
decision. It calls the existing factory-reset coordinator after a continuous
hold, once per press, and requires release after boot. Short/released presses
do not accumulate. Millisecond-counter rollover is tested.

GPIO0 is a strapping pin: do not hold BOOT during power-on/reset. This adapter
does not implement physical flash/BLE bond erasure or claim factory reset works.
The storage adapter must report actual erasure completion before provisioning
restarts; asynchronous BLE bond deletion must not be reported as already done.

Before device acceptance: agree duration; integrate real Wi-Fi vault/SDK and BLE
bond removal; compile the full selected firmware; test short press, long press,
boot-held button, reset failure and reboot persistence with Dheekshith.

Independent tester/date:
Approved hold duration:
Firmware/board:
Actual results:
RTM acceptance:

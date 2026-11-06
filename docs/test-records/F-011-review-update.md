# VAMC-F-011 software verification — 7 November 2026

Member: Dhanush S S. Cross-tester: G S Dheekshith. TC-CONN-01.
Acceptance remains guided device setup within five minutes; voice commands
must not depend on internet connectivity.

The portable coordinator now owns bounded copies of submitted credentials,
clears them on all terminal/cancel paths, rejects oversized strings, stops
after timeout and prevents late connection events from persisting credentials.
The fake store copies inputs synchronously before coordinator memory is cleared.
Credential stores/adapters must not retain caller pointers.

Nine focused host tests passed. Full regression and hosted CI results belong
in the PR. No actual AP, Wi-Fi connection, flash dump, radio pairing or five-
minute device test is established by these tests. A real portal/network adapter
and the newly approved passphrase-unlocked persistence are separate integration
work. Original TC-CONN-01 actual result remains blank, status T not A.

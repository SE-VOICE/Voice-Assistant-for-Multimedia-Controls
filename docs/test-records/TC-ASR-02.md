# TC-ASR-02 — command matching for exact and near-synonym phrases

Requirement: VAMC-F-005
Owner: Harsh Mannan
Independent tester: Chiranthan Shankar
Status: T (ready for transcript-level cross-test; not accepted)

## Part A — transcript level (native, no hardware)

1. In `firmware`, run `pio test -e native -f test_VAMC_F_005`.
2. Confirm all tests pass, including exact phrases for all 7 commands, the
   issue #5 user story ("turn it up" and "volume up"), near-synonyms for every
   command, near-match correction, and consistency with
   `CommandMapper::from_string`.
3. Add any phrase you expect users to say that the matcher gets wrong as a new
   failing test, and report it on the PR.

## Part B — spoken phrases (blocked)

Blocked on VAMC-F-004 (cloud ASR streaming) and the board selection. When
unblocked: speak each exact and near-synonym phrase from Part A through the
device and verify the matched command.

Expected result from the STP: Each phrase maps correctly to one of the 7 commands.

Actual result: __________
Test date: __________
Tester: __________
Board, host, and firmware version: __________
Evidence link: __________

Passing native tests does not fill in the actual result.

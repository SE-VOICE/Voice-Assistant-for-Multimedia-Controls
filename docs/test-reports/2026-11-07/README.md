# Team 3 testing reports — 7 November 2026

Application source: `91d5a74d815a2daff04afd896e4f135877d552b9`. Actual execution: **07 November 2026, 01:50 IST (UTC+05:30)**, not backdated to
the course's 2–6 November testing window.

1. [Unit Test Report](Unit_Test_Report.md)
2. [System Validation Report](System_Validation_Report.md)
3. [Code Coverage Report](Code_Coverage_Report.md)
4. [Branch Coverage Report](Branch_Coverage_Report.md)

**Automated result:** 93/93 C++ cases (87 unit/contract, 6 integration) and 10/10
optional Python methods passed. Instrumented reruns repeat the existing cases.

**Coverage:** 12 portable C++ implementation files: 662/691 lines (**95.80%**),
457/518 branch outcomes (**88.22%**). Optional Python prototype: 50/120 statements
(**41.67%**), 14/36 branch outcomes (**38.89%**). Read the reports' explicit
exclusions; these percentages do not cover the entire product or hardware.

**System acceptance remains pending.** No new independent acceptance, flash,
physical voice/media experiment, battery test or OTA execution is claimed.
ESP32 build configuration and the final model/adapters remain incomplete.

Raw logs, annotated coverage, JSON, snapshots, source hashes and environment
details are under [evidence](evidence/). Reproduction runner:
`scripts/measure_coverage.py` (macOS). OpenSSL/Python and Clang metrics stay separate.

Prepared under Dhanush's integration/regression responsibilities (INT-02).
Independent team review and DOC-01 baseline reconciliation remain pending.
No release freeze, issue closure or RTM Accepted status is authorized by this bundle.

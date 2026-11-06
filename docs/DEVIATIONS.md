# Approved scope adjustments

## SR-003 boot unlock — 7 November 2026

- ID: DEV-SR003-BOOT-UNLOCK.
- Documents: SAD credential-protection decision and VAMC-SR-003 setup behavior.
- Difference: the user enters a passphrase locally on every boot; the derived
  key exists only in RAM. Credentials remain unavailable until unlock.
- Why: Dhanush explicitly selected this design instead of embedding a key beside
  encrypted credentials. No automatic unlock or irreversible security setup.
- Impact: changes boot/setup interaction; setup duration, PBKDF2 duration and
  flash confidentiality still need independent target measurement. No requirement
  ID, acceptance threshold or test acceptance status is changed.
- Approval: Dhanush's explicit answer, 7 November 2026: "Use a passphrase entered
  at each boot; keep the key only in RAM".
- Evidence: credential-protection-decision.md and test-records/TC-SEC-03.md.

## DR-01 scope clarification — 24 October 2026

- Documents: AGENTS.md section 11 versus backlog/backlog.csv DR-01.
- Difference: AGENTS includes a firmware skeleton; the CSV lists Project linkage,
  visible backlog, PR template and branch protection only.
- Approval: Dhanush explicitly replied “yeah go on” after being asked to include
  the firmware foundation in DR-01.
- Implementation: shared headers, module boundary placeholders, native test
  environment, inert Arduino entry points, and PR template.
- Impact: adds prerequisite scaffolding; no feature acceptance criteria or IDs
  changed. Original DR-01 acceptance is not declared complete.
- Framework: Dhanush approved PlatformIO + Arduino-ESP32.
- Board: Dhanush deferred selection. The esp32dev build environment is withheld
  until selection, so an ESP32 build and hardware testing remain blocked.
- Other open decisions (pairing, credential protection, retry count, HID mapping,
  cloud, wake engine, OTA) remain unresolved; placeholders implement none of them.

# Approved scope adjustments

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

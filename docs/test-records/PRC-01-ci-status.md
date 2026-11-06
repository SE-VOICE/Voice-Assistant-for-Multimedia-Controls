# PRC-01 — native CI rollout

Owner: G S Dheekshith. CI maintenance implemented by Codex for Dhanush under
the team lead's request to review and resolve available project work.

The workflow runs PlatformIO's native firmware suite on every PR and push to
main. Python and PlatformIO versions are explicit. Repository write permission
is not granted to the workflow. Companion tests retain their separate workflow.

This is partial PRC-01 progress, not final story acceptance: platformio.ini
still has no approved esp32dev build environment, so CI cannot yet build the
full target firmware. No flashing or hardware behavior is claimed. Branch
protection and independent test/RTM acceptance are still separate work.

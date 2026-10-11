# TinyControlBoard — Documentation Set

## System overview

TinyControlBoard is the front-panel control and power-sequencing system for a
Raspberry-Pi-based moOde Audio streamer. The [system architecture document](01-system-architecture.md)
is the canonical source for system boundaries, component responsibilities, communication
flows, deployment, quality requirements, and the control/audio diagrams.

Use the focused guides below for implementation and service detail. Keep architectural
descriptions and system-level diagrams in the architecture document; these guides are the
authoritative references for their respective implementation areas.

## Where to start

1. New to the project? Read [01-system-architecture.md](01-system-architecture.md) first.
2. Setting up a dev machine? Read [07-development-environment.md](07-development-environment.md).
3. Working on firmware? [03-esp32-developer-guide.md](03-esp32-developer-guide.md).
4. Working on the Raspberry Pi side? [04-rpi-developer-guide.md](04-rpi-developer-guide.md).
5. Working on the Android app? [02-android-developer-guide.md](02-android-developer-guide.md).
6. Changing the wire protocol? [05-communication-protocol.md](05-communication-protocol.md) — read this
   before touching `uartProtocol.hpp`, `protocol.py`, or `BleProtocol.kt`.
7. Working on wiring/GPIO/relays? [06-hardware-interface.md](06-hardware-interface.md).
8. Known gaps and cross-component consistency checks: [08-quality-check.md](08-quality-check.md).
9. Adding a new button or action: [09-adding-button-actions.md](09-adding-button-actions.md)
    (a matching `/add-button-action` prompt lives in `.github/prompts/`).

## Source snapshot

- **Repository**: single monorepo at `d:\Dev\TinyControlBoard` — the Android app, ESP32
  firmware, and Raspberry Pi scripts are all subtrees of this one repository/commit
  (there are no separate Android/RPi/ESP32 repos).
- **Branch**: `feature/library-improvements`
- **Commit**: `402f526` (2026-09-17)
- **Snapshot/inspection date**: 2026-09-18
- Working tree had a handful of locally deleted-but-uncommitted legacy doc files
  (`docs/project-guide.md`, `docs/protocol-reference.md`, `docs/add-button-how-to.md`,
  two old exported `.docx`/`.md` files) at inspection time — not part of this
  documentation set's inputs.

Pre-existing narrower documents (`architecture.md`, `button-command-map.md`,
`power-sequencing.md`, `wiring-reference.md`, `wifi-transport-plan.md`,
`stream-dac-front-panel-user-guide.md`) remain in this folder and are referenced
where they contain corroborating detail; this new numbered set supersedes them as
the primary developer-facing reference. End-user manuals (`AndroidAppUserManual.md`,
`user-manual.md`, `RPI4_SETUP.md`) now live under `docs/userDocumentation/`.

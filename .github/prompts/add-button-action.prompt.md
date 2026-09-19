---
description: "Add a new physical button or button behavior to the TinyControlBoard ESP32 firmware (Momentary, Toggle, or Timed action, plus optional local-only handling)"
name: "Add Button Action"
argument-hint: "Button name, behavior (Momentary/Toggle/Timed/local-only), and target command name(s)"
agent: "agent"
---
You are adding a new button or a new action to the TinyControlBoard ESP32-S3
firmware. Follow [docs/09-adding-button-actions.md](../../docs/09-adding-button-actions.md)
exactly — it is the canonical, verified walkthrough for this codebase. Do not
invent alternative wiring mechanisms.

Ask the user (if not already given) which of these shapes the new button needs:

1. **Momentary / Simple** — one fixed command per press, no state.
2. **Toggle** — alternates ON/OFF each press, LED flips, normalized to one wire
   command with a boolean parameter.
3. **Timed** — measures hold duration and sends it as a parameter.
4. **Local-only** — stays on the ESP32 (relay/brightness/system), never sent
   over UART.

Then, using the current source as ground truth (re-read the files below before
editing — do not assume the doc's example code is verbatim up to date):

- [lib/protocol/uartProtocol.hpp](../../lib/protocol/uartProtocol.hpp) — add the `CMD_*` constant(s).
- [lib/board/boardButtonIds.hpp](../../lib/board/boardButtonIds.hpp) — reserve the button ID at the END of the list, bump `k_count`.
- [lib/app/ControlBoardActionRegistry.cpp](../../lib/app/ControlBoardActionRegistry.cpp) — add one row to `k_buttons[]`.
- Toggle only: [lib/app/ActionUartDispatcher.cpp](../../lib/app/ActionUartDispatcher.cpp) — add a row to `k_toggleTable[]`.
- Local-only: [lib/app/ActionCommandRoutingPolicy.hpp](../../lib/app/ActionCommandRoutingPolicy.hpp) (classify), [lib/app/ActionFactory.cpp](../../lib/app/ActionFactory.cpp) (map new route if needed), and the relevant class in [lib/app/commands/](../../lib/app/commands/).
- [lib/protocol/commandCatalog.cpp](../../lib/protocol/commandCatalog.cpp) — add display name(s) for logging.
- If UART is involved, add handling on the Raspberry Pi side (`scripts/rpi/home/antho/command_ids.py`, `playback_commands.py`, and `uart5_listener.py`'s `COMMAND_HANDLERS`).
- Update [docs/button-command-map.md](../../docs/button-command-map.md).

Requirements:
- Keep new button IDs additive (never renumber existing ones — it shifts LED bit positions).
- Match existing naming conventions (`CMD_<NAME>`, `CMD_<NAME>_ON/OFF`, `k_<camelCase>`).
- After editing, run the `PlatformIO Build` task and fix any compile errors before finishing.
- Summarize exactly which files you changed and why, mapped back to the checklist in docs/09-adding-button-actions.md §8.

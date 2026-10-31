# Tasks

## In Progress

- [X] 0.5 — Fix rotary encoder function
- [X] 1a — Set and verify basic controls: CTLBoard → RPi4 commands
- [X] 1b — Set and verify toggle controls
- [X] 1c — Set and verify specialised controls (rotary controller)
- [X] 4 — Implement and validate inbound data: RPi → CTLBoard UART (`actionProcessor.cpp` scaffold)
- [X] 2 — Find method to check if RPi4 has power
- [X] 3 — Verify relays are working correctly

  - [X] There is an issue where the relays close on start up because the level translation chip is the incorrect type this will be fixed in the next version
- [X] 5 — Log connections

## Future Considerations

- [ ] Upgrade protocol checksum from 1-byte sum to CRC-16 (implement alongside item 4), this will be fixed on a future update

# Open Questions – Six-Channel Relay Board

| # | Question | Why it matters |
|---|---|---|
| 1 | Which relay contact does each header pair give (normally open assumed [I])? | Fail-safe state of connected equipment |
| 2 | What supplies J1 (voltage and source) and what are the J2 sources (logic level)? | Datasheet input limit is 5.5 V; operate needs ≥ 1.65 V |
| 3 | Is the J2 reversed numbering intentional, and is it handled in firmware or wiring? | Wrong relay operated |
| 4 | What is connected to J4/J5 (voltage, current)? | Contact rating, pin-header spacing, mains suitability |
| 5 | Is the red LED Vf and brightness acceptable? (Vf not verified) | Indicator current |
| 6 | Is one 100 nF capacitor enough for six relays? | Supply dips and chatter |
| 7 | Should the supply net be labelled +5V and a power flag added? | Clean ERC |
| 8 | Remove or fix the dangling track and vias flagged by DRC? | Tidy-up only |
| 9 | Which release of the board was built (no revision on the schematic)? | Traceability |
| 10 | Measured coil resistance, coil current and contact resistance on a built board | Confirms section 3 calculations |

# Open Questions – CapacitorBank

| # | Question | Why it matters |
|---|---|---|
| 1 | Was the requested August 2025 `CapacitorBank` folder intentionally saved as an empty placeholder, or were the real schematic and PCB lost? | Determines whether the requested folder can ever be a service source |
| 2 | Should the service guide for “Capacitor Bank” actually track `CapBank2\CapacitorBank` instead of the requested folder? | Source control and maintenance baseline |
| 3 | Does the older section in [`..\..\PowerSupplySubsystem.md`](../../PowerSupplySubsystem.md) describe `CapBank2` rather than the requested folder? | Prevents technicians using the wrong document |
| 4 | Is there a released schematic/PCB revision between August 2025 and October 2026 that should supersede both folders? | Traceability |
| 5 | For the later `CapBank2` board, what exact terminal-block manufacturer part number and current rating were intended for J1 and J2? | Connector derating and service replacement |
| 6 | For the later `CapBank2` board, should the auto-generated nets be renamed explicitly to `Vin` and `Vout` in the schematic? | Clearer service diagnostics |
| 7 | For the later `CapBank2` board, was R3 = 160 kohm intentionally chosen for about 0.69 A typical current limit? | Startup behaviour and overload margin |
| 8 | For the later `CapBank2` board, is the lack of a bleed resistor acceptable for service handling? | Residual charge risk after power-off |
| 9 | Can the capacitor manufacturer PDF for `875075361005` be archived locally with the project? | Independent verification of ESR, ripple and lifetime data |
| 10 | What physical hardware revision is actually fitted in the system being serviced? | Determines which file set and spare parts are correct |

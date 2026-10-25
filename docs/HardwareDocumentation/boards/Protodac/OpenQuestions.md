# Open Questions – Protodac

No older local `service-guide\boards` summary file existed for Protodac, so there was no prior local summary to diff against the KiCad source.

| # | Question | Why it matters |
|---|---|---|
| 1 | What is the exact part number and verified datasheet for the fitted `TDA1387x8` module? | Input thresholds, supply current, output level and absolute maximum ratings are otherwise not established from the project files |
| 2 | Is `BC1` (module pin 4) internally linked or otherwise intentionally unused on the fitted module? | KiCad leaves pin 4 open; a module variant that expects it to be driven may not work |
| 3 | What is the intended digital source and voltage level for J6 and J1/J2/J3? | Full pin-level compatibility of the input interface could not be verified from the files alone |
| 4 | Must J6 and J1/J2/J3 be treated as mutually exclusive inputs in the final product? | The connector sets are wired in parallel through separate 430R resistors |
| 5 | What load impedance and downstream circuit are intended for J4? | Final output level, LF roll-off and distortion margin depend on the receiving load |
| 6 | Which voltage grades and exact series are intended for C1-C4? | The KiCad values show capacitance, but not the final voltage selection or physical margin |
| 7 | How should schematic Rev. 0.2 be mapped to PCB silkscreen V1.1? | Traceability between documentation, BOM and built hardware |
| 8 | Are the 16 `silk_over_copper` warnings acceptable for manufacture and service readability? | Several markings around the module/output area may be clipped or obscured |
| 9 | What is the required in-service test method for verifying the DAC module without risking speakers/headphones? | The board has no mute relay and only one explicit test point |
| 10 | What output DC transient is acceptable at power-up and power-down on J4? | The outputs are AC-coupled but no measured behaviour is available from hardware |

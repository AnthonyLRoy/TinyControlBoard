# Open Questions – Pi4 Hat (PiHat)

| # | Question | Why it matters |
|---|---|---|
| 1 | Is J7 really intended to power the Raspberry Pi 5 V pins, or are the two 5 V nets intentionally separate? | Main service/safety ambiguity; the old summary and current KiCad disagree |
| 2 | Were J1, J4, J5, J6, J8, J13 and J14 intentionally removed from the PCB, or is the PCB incomplete? | Schematic/PCB consistency and manufacturability |
| 3 | Should J14 be GND, +5 V, or removed? It is labelled `5v pin` but is connected only to isolated `GND2`. | Prevents wiring mistakes |
| 4 | Are UART, `ESP DRY`, `RPI DRY` and `Mon PWM` meant to be real external interfaces in a later revision? | They are named on the schematic but not broken out on the PCB |
| 5 | Is a HAT ID EEPROM intentionally omitted? | Automatic Raspberry Pi HAT identification is not implemented in the current files |
| 6 | Are the DRC-reported malformed outline, J12 ground connection and other unconnected items known/accepted? | Manufacturing and service confidence |
| 7 | Which connectors are actually fitted on the built hardware, especially the schematic-only power/ground connectors? | Field servicing and spare-parts list |
| 8 | What cable type and length are expected on the U.FL signal outputs? | Direct 3.3 V GPIO export has no buffering or line driver |
| 9 | Does the built board include any off-file bodge wires or copper links joining the two 5 V domains? | Could explain a working board despite the KiCad split |
| 10 | Which statements in the old `RaspberryPiInterfaceBoard.md` summary should be retained, and which are obsolete? | Document control and traceability |

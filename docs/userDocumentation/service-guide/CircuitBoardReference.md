# Custom Circuit Board Reference

[Back to the Service Guide](../ServiceGuide.md) · [DAC and audio subsystem](DacSubsystem.md) · [Power supply subsystem](PowerSupplySubsystem.md) · [System wiring and commissioning](SystemWiring.md)

This reference collects the existing board descriptions, connector information, test points, and troubleshooting notes. Treat details marked uncertain as unverified; check the current KiCad schematics and component datasheets before wiring or servicing a board.

## Boards

- [Tiny Control Board for Raspberry Pi 4](boards/TinyControlBoard.md): main control board with buttons, LEDs, I2C expander, and Pi/ESP32 serial link.
- [Raspberry Pi Interface Board](boards/RaspberryPiInterfaceBoard.md): 40-pin header board with filtered 5 V and I2S/I2C signal outputs.
- [Power Relay Board](boards/PowerRelayBoard.md): power switching board (schematic and layout).
- [6-Channel Relay Board](boards/SixChannelRelayBoard.md): six-relay board driven by a ULN2003LV array.

## Engineering and service documents (generated from the KiCad projects)

Each document is built from the KiCad source, with every value tagged by evidence, and has a PDF and an open-questions list. Nothing was measured on hardware.

- [Tiny Control Board](boards/TinyControlBoard/TinyControlBoard-EngineeringAndService.md)
- [Six-Channel Relay Board](boards/SixChannelRelayBoard/SixChannelRelayBoard-EngineeringAndService.md)
- [Pi4 Hat](boards/PiHat/PiHat-EngineeringAndService.md)
- [Protodac](boards/Protodac/Protodac-EngineeringAndService.md)
- [SE to Diff board](boards/SeToDiffBoard/SeToDiffBoard-EngineeringAndService.md)
- [Diff to Single-Ended board](boards/DiffToSingleEndedBoard/DiffToSingleEndedBoard-EngineeringAndService.md)
- [Relay OUT Signal Switch board](boards/RelayOutSignalSwitchBoard/RelayOutSignalSwitchBoard-EngineeringAndService.md)
- [Capacitor Bank](boards/CapacitorBank/CapacitorBank-EngineeringAndService.md) (the requested KiCad folder is empty; see its open questions about CapBank2)

## DAC subsystem

- [Single-Ended to Balanced Line Driver](dac-subsystem/SingleEndedToBalancedLineDriver.md)
- [Differential-to-Single-Ended Output Board](dac-subsystem/DifferentialToSingleEndedOutputBoard.md)
- [ProtoDAC Board](dac-subsystem/ProtoDACBoard.md)
- [DAC Output Relay Switch](dac-subsystem/DACOutputRelaySwitch.md)
- [Input Selector Board](dac-subsystem/InputSelectorBoard.md)
- [Analogue Audio](dac-subsystem/AnalogueAudio.md)


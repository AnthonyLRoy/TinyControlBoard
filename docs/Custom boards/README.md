# Custom Boards

Index of the custom PCBs designed for the Tiny Control Board system. Each page contains renders, an overview table, and the schematic.

| Board | Description |
|---|---|
| [TinyControlBoard](tinyControlBoard/TinyControlboard.md) | The main controller. A compact ESP32-S3 board that scans a 16-button front panel, drives 16 constant-current LEDs, level-shifts control lines to external equipment (DAC, screen, relays) and communicates with the Raspberry Pi. Powered from 5 V with USB-C programming. |
| [Capacitor Bank](<Capacitor Bank/Capacitor Bank.md>) | A power-distribution board built around a TPS2556 power switch, used to supply and switch a bank of capacitors. Includes test points; no MCU. |
| [Dac_output_ Relay](<Dac_output_ Relay/Dac_output_ Relay.md>) | A relay board using a ULN2003 driver to switch two balanced and two single-ended stereo inputs to balanced and single-ended outputs. All lines switch together. Runs from 5 V. |
| [Differential to Single-Ended](Differentia_to_SingleEnded/Differentia_to_SingleEnded.md) | An audio board using OPA1692 dual op-amps to convert balanced (differential) audio signals to single-ended. Needs +5 V / -5 V supplies. |
| [Single-Ended to Differential](Single_ended_to_Diff/Single_ended_to_Diff.md) | An audio board using two 1646S08-U devices to convert single-ended audio to balanced (differential) outputs. Needs +5 V / -5 V supplies. |
| [Power Relay Board](Power_Relay_Board/Power_Relay_Board.md) | A six-channel relay board using a ULN2003 driver with EE2-5NU relays, for switching power and other loads from control signals. |
| [PowerSupplyUCIndicators](PowerSupplyUCIndicators/PowerSupplyUCIndicators.md) | A two-channel indicator board with 2N3904 transistor stages, 1k trimmers, and six-pin host connectors for sense, power, and on/off signals. |
| [ProtoDac](ProtoDac/ProtoDac.md) | An eight-channel TDA1387 DAC module carrier with digital audio inputs and stereo analog output terminals. Powered from 5 V. |
| [RPI-Power-I2s-Connector](RPI-Power-I2s-Connector/RPI-Power-I2s-Connector.md) | A passive adapter with a 2x20 Raspberry Pi GPIO header that distributes 5 V power and breaks out the I2S audio signals to the DAC boards. |

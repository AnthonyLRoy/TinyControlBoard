# OpenQuestions - DiffToSingleEndedBoard

| # | Question | Why it matters |
|---|---|---|
| 1 | Was **R18 = 5 kOhm** intentionally omitted from the PCB and production outputs, or is the PCB out of sync with the schematic? [K] | The left-channel schematic and the built PCB implement different transfer functions. |
| 2 | Should the **right channel** have a resistor equivalent to R18 from U3B+ to `GND 2`? [K][I] | Without it, the stage is not a conventional differential receiver and common-mode rejection is not established. |
| 3 | Is the intended transfer actually `Vout ~= 1.5*V+ - 0.5*V-` on both channels, or was a `0.5*(V+ - V-)` receiver intended? [C][I] | Defines gain, channel balance and noise / hum rejection. |
| 4 | Are the **J8 silkscreen labels** reversed relative to the actual nets (`Rout` on top, `Lout` on bottom)? [K][I] | A harness built from the silkscreen could swap left and right outputs. |
| 5 | What is the intended role of **J1 / GND3**? [K][I] | Chassis bonding and screening strategy are otherwise undocumented. |
| 6 | Is the **R15 // C11** link between `GND` and `GND 2` deliberate, and how should it be tied into the rest of the DAC chassis? [K][I] | Affects hum, EMC behaviour and fault-finding. |
| 7 | Are the **10 dangling vias** and **2 isolated copper pours** in the chassis-ground region intentional? [K] | They should be cleaned up before manufacturing release if accidental. |
| 8 | Should the multiple output net names (`se out L` / `Lout`, `se out r` / `Rout`) be rationalised? [K] | Avoids documentation and debug confusion. |
| 9 | Which name should be treated as authoritative for released documentation: **Diff > Single Version 2.0**, **XLR to single ended**, or **Differential-to-Single-Ended Output Board**? [K] | Traceability and service documentation consistency. |
| 10 | What are the verified connector ratings for **J2** and **J6-J8** from the chosen purchased parts? [M] | Required if the board is to be released for manufacture or field service. |

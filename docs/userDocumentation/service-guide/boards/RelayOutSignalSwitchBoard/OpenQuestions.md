# Open Questions – Relay OUT Signal Switch board

| # | Question | Why it matters |
|---|---|---|
| 1 | The PCB puts **D2 anode on `/switch power`**, but the schematic puts it on the **control-input net**. Which behaviour was intended? | Current source files disagree; service and production need one source of truth |
| 2 | The PCB DRC reports an **unconnected item at D2 pad 2**. Is this a real manufacturing defect in the released layout? | D2 may never indicate relay state unless repaired |
| 3 | Confirm on hardware that **relay de-energised = input set 1** and **relay energised = input set 2**. | Prevents source-selection errors in commissioning and service |
| 4 | What is the intended **external earth/chassis bond** for J2 `Earth`? | J2 is isolated on the PCB, while audio ground reaches `GND2` only through `R3 = 10 ohm` |
| 5 | Is **C1 = 100 nF** alone acceptable for three `5 V` relay coils, or should bulk capacitance be added off-board or on-board? | Affects relay stability and supply dip margin |
| 6 | Confirm the manufacturer specification of connector part **1725669** (current, voltage, pitch and mating style). | The KiCad part number is known, but the manufacturer page could not be fetched here |
| 7 | Is J1 always driven from **3.3 V or 5 V logic only**? | IC1 input absolute maximum is `5.5 V` |
| 8 | Is the board intended only for **line-level audio signals**? | The relay itself is rated far above line level, but system use, connector rating and safety spacing were not established |
| 9 | Should the **Earth** net remain isolated, or was a PCB bond/link omitted? | Hum, shield-current and safety behaviour depend on this |
| 10 | No older service-guide summary matching this board name was found. Is there an unindexed legacy note elsewhere that should point to this document? | Avoids parallel, contradictory maintenance notes |

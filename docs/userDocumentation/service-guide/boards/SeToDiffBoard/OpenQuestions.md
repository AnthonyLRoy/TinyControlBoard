# Open Questions: SE to Diff board

| # | Question | Why it matters |
|---|---|---|
| 1 | What maximum single-ended input level is expected from the upstream DAC or buffer? | The board clips at about **1.5 Vrms guaranteed** to **2.0 Vrms typical** input because the THAT1646 gain is +6 dB on only +5 V / -5 V rails [C][D]. |
| 2 | Is the upstream source genuinely low impedance? | The effective input load is only about **4.52 kOhm per channel** [C][D][K]. |
| 3 | How should J2/J3 pin 2 (`GND`) be used in the finished product: cable shield, signal return, chassis bond, or left open? | `GND` is not the same net as J4 pin 2 (`GND2`); they are linked only by **R5 10 ohm || C8 100 nF** [K]. |
| 4 | Can J2/J3 ever be wired to equipment that may present phantom power, surge energy or long external cables? | The PCB does not include the full THAT1646 Figure 8 protection network (no 100 pF-to-chassis shunts and no 1N4004 rail clamps) [D][K]. |
| 5 | Are the resistor manufacturer fields in KiCad intentionally stale, or should the BOM metadata be corrected? | R1-R4 all carry the same **300 ohm** Panasonic metadata even though the schematic values are **3 kOhm** and **47 kOhm** [K]. |
| 6 | Can the Phoenix 1725669 connector ratings be manually re-checked against the manufacturer datasheet before release? | Automated retrieval of the connector product page was blocked during this review, so this document used KiCad part metadata for connector ratings [K][M]. |
| 7 | Measure one built board for rail current, differential swing, DC output offset, and noise. | Confirms the desk calculations and checks that assembly matches the KiCad intent [M]. |

## Existing summary note versus KiCad source

A repository search found no older board document under `boards\`; the only prior note found was [`dac-subsystem\SingleEndedToBalancedLineDriver.md`](../../dac-subsystem/SingleEndedToBalancedLineDriver.md) [K].

| Item | Older note | KiCad / datasheet review |
|---|---|---|
| Input impedance wording | Says the `47 kOhm // 220 pF` network "sets the input impedance" | Effective channel input impedance is about **4.52 kOhm** because the THAT1646 input itself is about **5 kOhm** [C][D][K]. |
| Headroom warning | Not stated | A **2 Vrms** source may overdrive the board on +5 V / -5 V rails [C][D]. |
| KiCad BOM metadata quality | Not stated | R1-R4 manufacturer fields appear inconsistent with the schematic values [K]. |

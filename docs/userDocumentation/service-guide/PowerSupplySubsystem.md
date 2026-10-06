# Power Supply Subsystem

[Back to the Service Guide](../ServiceGuide.md) · [Circuit board reference](CircuitBoardReference.md) · [System wiring and commissioning](SystemWiring.md)

> **Mains-voltage hazard:** This subsystem includes mains-connected transformers. Do not work on internal wiring while connected to mains. Protective-earth bonding, fusing, insulation, and wiring must be verified by a person qualified to work safely with hazardous voltages. Do not rely on this draft as a complete mains-wiring procedure.

## Supply Modules

### LinearPi Pro

Two LinearPi Pro modules are used. See Ian Canada's [LinearPi documentation](https://github.com/iancanada/DocumentDownload/blob/master/LinearPi/LinearPiMkIIDual.jpg).

<img src="../../user-manual-media/media/service-guide/linearpi-pro-dual-power-supply.png" style="width:6.26806in;height:4.01875in" />

### UcPure

See Ian Canada's [UcPure manual](https://github.com/iancanada/DocumentDownload/blob/master/UltraCapacitorPowerSupply/UcPure/OLD/UcPureMkIIManual.pdf).

<img src="../../user-manual-media/media/service-guide/ucpure-power-supply-board.png" style="width:6.26806in;height:4.22222in" />

Other power components include an LHY 5 V supply, an LED board, and transformers.

## Raspberry Pi DAC Power Board

<img src="../../user-manual-media/media/service-guide/raspberry-pi-dac-power-schematic.png" style="width:6.26806in;height:4.29792in" />

<img src="../../user-manual-media/media/service-guide/raspberry-pi-dac-power-board.png" style="width:5.25434in;height:4.58546in" />

## Current-Limited Power Switch and Capacitor Bank

### Circuit Schematic and Board


<img src="../../user-manual-media/media/service-guide/capacitor-bank-photo.png" style="width:6.26806in;height:4.40625in" />

<img src="../../user-manual-media/media/service-guide/capacitor-bank-schematic.png" style="width:6.26806in;height:4.40625in" />

A DC supply passes through a TPS2556-Q1 current-limited power switch into a large capacitor bank and an output connector. The switch limits inrush and overload current.

### What It Does

- **IC1 (TPS2556QDRBTQ1)** is an adjustable current-limit high-side switch. It is always enabled and limits output current to a level set by R3.
- Twenty parallel capacitors on the output (C1 to C18, C20, C21) form a bulk store.
- A fault flag (`FAULT`) goes low when the switch is in overcurrent or thermal shutdown.

**Power path:** J1 (Vin) → IC1 → Vout → J2, with the capacitor bank on Vout

### Connectors

| Ref | Pin | Net |
|---|---|---|
| J1 | 2 | Vin (supply +) |
| J1 | 1 | GND |
| J2 | 1 | Vout (protected +) |
| J2 | 2 | GND |

### Components

| Ref | Value | Function |
|---|---|---|
| IC1 | TPS2556QDRBTQ1 | Current-limited power switch |
| R1 | 100k | Pull-up on `FAULT` (open-drain, active low) |
| R2 | 10k | Pull-down on EN to GND; keeps the switch enabled |
| R3 | 160k | ILIM resistor, sets the current limit |
| R4 + D1 | 1k + LED | Power-present indicator on Vin |
| C22 | 47 µF | Input bulk capacitor |
| C19 | 0.1 µF | Input decoupling |
| C1 to C18, C20, C21 | 875075361005 (x20) | Output bulk capacitors, all in parallel |
| TP1 | Test point | Vout |
| TP2 | Test point | Vin |
| TP3 | Test point | ILIM node |

#### IC1 Pin Connections

| Pin | Name | Connection |
|---|---|---|
| 1 | GND | GND |
| 2, 3 | IN_1, IN_2 | Vin |
| 4 | EN | GND through R2 |
| 5 | ILIM | R3 to GND, TP3 |
| 6, 7 | OUT_1, OUT_2 | Vout |
| 8 | FAULT | Pulled to Vin by R1 |
| 9 (EP) | Thermal pad | GND |

### Operation

1. With Vin applied, D1 lights.
2. EN is held low, so IC1 turns on and charges the output bank.
3. If the load or bank charge current reaches the limit set by R3, IC1 holds the current at that limit instead of passing more.
4. In a sustained overload or over-temperature condition, `FAULT` goes low and the switch protects itself.

### Quick Checks

| Check | Expected |
|---|---|
| J1 pin 2 to pin 1 | Supply voltage, within 2.5 V to 6.5 V |
| D1 | Lit |
| TP2 | Same as supply voltage |
| TP1 (Vout) | Close to Vin once the bank has charged |
| IC1 pin 8 (`FAULT`) | High (about Vin) in normal operation |
| IC1 pin 4 (EN) | Low (0 V) |

### Quick Troubleshooting

| Symptom | Check |
|---|---|
| D1 off | Supply on J1, polarity, then R4 and D1 |
| No Vout | Vin at IC1 pins 2 and 3, EN at 0 V, `FAULT` state, then IC1 |
| `FAULT` low | Overcurrent or over-temperature; check for a short or an overloaded output, then remove the load |
| Vout rises slowly or cycles on and off | Bank charging at the current limit; IC1 may be heating and restarting. Check for shorts, then check that the load is not drawing current during start-up |
| Vout low under load | Load current is above the limit set by R3 |
| IC1 very hot | Output short, load above the limit, or a large bank charging at the limit |
| Limit looks wrong | R3 value and its connection to ILIM (TP3), then the datasheet table |

### Notes

- **Current limit:** R3 = 160k. For reference, the datasheet lists 61.9k as about 1.8 A typical, so 160k should be roughly 0.7 A. This is an estimate; confirm with the datasheet equation and tolerance table.
- **EN polarity:** the TPS2556 enable is active low, so pulling EN to GND through R2 leaves the switch on permanently. Do not change this to a pull-up.
- **Input range:** 2.5 V to 6.5 V. Check that Vin and every output capacitor rating fit this range.
- **Bank capacitance:** the total is 20 times the single-part value (see the BOM). Charge time is roughly total capacitance x Vin / current limit.
- **FAULT** is not routed to a connector or test point. Probe IC1 pin 8 or the right-hand end of R1.
- Pin and net details were read from a screenshot. Confirm against the KiCad schematic before using them for repairs.

## Power Supplies

*Supply currents and distribution details are still to be confirmed; replace placeholder values with measured or verified specifications before construction.*

### Supply Overview

<table>
<colgroup>
<col style="width: 86%" />
<col style="width: 6%" />
<col style="width: 6%" />
</colgroup>
<thead>
<tr>
<th>Rail</th>
<th>Voltage</th>
<th>Current</th>
</tr>
</thead>
<tbody>
<tr>
<td>Digital</td>
<td>5V</td>
<td>xxx mA</td>
</tr>
<tr>
<td>DAC Core</td>
<td>3.3V</td>
<td>xxx mA</td>
</tr>
<tr>
<td>Analogue</td>
<td>±15V</td>
<td>xxx mA</td>
</tr>
</tbody>
</table>

#### Bill of Materials

<table style="width:86%;">
<colgroup>
<col style="width: 17%" />
<col style="width: 15%" />
<col style="width: 12%" />
<col style="width: 41%" />
</colgroup>
<thead>
<tr>
<th style="text-align: center;">ProductID</th>
<th style="text-align: center;">Description</th>
<th style="text-align: center;">URL</th>
<th style="text-align: center;">Image</th>
</tr>
</thead>
<tbody>
<tr>
<td style="text-align: center;">723-BCAP0325P270S19</td>
<td style="text-align: center;">Super Capacitor 325F 2.7V (Maxwell/Eaton)</td>
<td style="text-align: center;"><a href="https://www.mouser.com/ProductDetail/723-BCAP0325P270S19">Mouser</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/maxwell-325f-supercapacitor-thumbnail.png" style="width:2.14553in;height:1.83562in" /></td>
</tr>
<tr>
<td style="text-align: center;">546-1182N6</td>
<td style="text-align: center;">Hammond Power Transformer 6V+6V 20VA</td>
<td style="text-align: center;"><a href="https://www.mouser.com/ProductDetail/546-1182N6">Mouser</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/hammond-1182-transformer-thumbnail.png" style="width:2.10274in;height:2.00823in" /></td>
</tr>
<tr>
<td style="text-align: center;">546-1182M9</td>
<td style="text-align: center;">Hammond Power Transformer 9V+9V 20VA</td>
<td style="text-align: center;"><a href="https://www.mouser.com/ProductDetail/546-1182M9">Mouser</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/hammond-1182-transformer-thumbnail.png" style="width:2.10274in;height:2.00823in" /></td>
</tr>
<tr>
<td style="text-align: center;">VTX-146-060-206</td>
<td style="text-align: center;">Vigortronix Power Transformer 6VA 2x6V</td>
<td style="text-align: center;"><a href="https://www.switchelectronics.co.uk/products/vtx-146-050-206-toroidal-transformer-50va-0-6v-vigortronix?currency=GBP&amp;country=GB&amp;variant=45694220206389&amp;utm_source=google&amp;utm_medium=cpc&amp;utm_campaign=Google%20Shopping&amp;stkn=bbf1d20e1ed7&amp;gad_source=1&amp;gad_campaignid=23321857875&amp;gbraid=0AAAAAqEgT0BQfVbHs8KwmvJlH6DAzkAyo&amp;gclid=Cj0KCQjwjIPSBhCCARIsABGyK7vJPmnLQInHhnEMVQb2GtcvyNNqW93mJx-GKBlvhf_wyRRO4Ft1ehoaAvjYEALw_wcB">switch Electronics</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/vigortronix-6va-transformer-thumbnail.png" style="width:1.73713in;height:1.7634in" /></td>
</tr>
<tr>
<td style="text-align: center;">FN9290-4-06</td>
<td style="text-align: center;">Schaffner EMC Power Line Filter 4A</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=FN9290-4-06">Mouser Search</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/schaffner-fn9290-power-filter-thumbnail.png" style="width:2.19082in;height:2.04795in" /></td>
</tr>
<tr>
<td style="text-align: center;">BK8-HTC-603M</td>
<td style="text-align: center;">Eaton/Bussmann Fuse Holder with Fuses</td>
<td style="text-align: center;"><a href="https://www.mouser.co.uk/ProductDetail/Eaton-Electronics/BK8-HTC-603M?qs=DRkmTr78QAR4GxQA20iXeg%3D%3D&amp;srsltid=AfmBOooluWFogsZrJNLhNb5vHrr31zGF7_oef6-Sheq1SS1CPsoGj9U4">Mouser</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">NAUSB3-B</td>
<td style="text-align: center;">Neutrik USB 3.0 Feed-Through Adapter, Black D-shape</td>
<td style="text-align: center;"><a href="https://www.neutrik.com/en/product/nausb3-b">Neutrik</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/neutrik-nausb3-b-usb-adapter.png" style="width:2.80822in;height:2.80822in" alt="NAUSB3-B" /></td>
</tr>
<tr>
<td style="text-align: center;">NAHDMI-W-B</td>
<td style="text-align: center;">Neutrik HDMI 2.0 Feed-Through Adapter, Black D-shape</td>
<td style="text-align: center;"><a href="https://www.neutrik.com/en/product/nahdmi-w-b">Neutrik</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/neutrik-nahdmi-w-b-hdmi-adapter.png" style="width:2.75343in;height:2.75343in" alt="NAHDMI-W-B" /></td>
</tr>
<tr>
<td style="text-align: center;">NF2D-B-0</td>
<td style="text-align: center;">Neutrik Phono/RCA Socket, Black D-shape (SPDIF)</td>
<td style="text-align: center;"><a href="https://www.neutrik.com/en/product/nf2d-b-0">Neutrik</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/neutrik-nf2d-b-0-spdif-connector.jpeg" style="width:2.83012in;height:2.78767in" alt="NF2D-B-0" /></td>
</tr>
<tr>
<td style="text-align: center;">NC3MDM3LBAG-1</td>
<td style="text-align: center;">Neutrik XLR 3-pole Male Receptacle, Black D-shape, Silver Contacts</td>
<td style="text-align: center;"><a href="https://www.neutrik.com/en/product/nc3mdm3lbag-1">Neutrik</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/neutrik-nc3mdm3lbag-1-xlr-connector.jpeg" style="width:2.71918in;height:2.71918in" alt="NC3MDM3LBAG-1" /></td>
</tr>
<tr>
<td style="text-align: center;">ENCOS24D2S65R</td>
<td style="text-align: center;">Panel Mount Ethernet Connector (RJ45 Coupler)</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=ENCOS24D2S65R">Mouser Search</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">CP30217MB</td>
<td style="text-align: center;">Panel Mount Optical (TOSLINK) Input Connector</td>
<td style="text-align: center;"><a href="https://uk.farnell.com/cliff-electronic-components/cp30217mb/fibre-optic-adapter-toslink-toslink/dp/3490624">Farnell</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/panel-mount-toslink-connector.png" style="width:1.99933in;height:2.41096in" /></td>
</tr>
<tr>
<td style="text-align: center;">PX0794/S</td>
<td style="text-align: center;">Bulgin PX0794 Circular Connector, Socket</td>
<td style="text-align: center;"><a href="https://www.mouser.co.uk/ProductDetail/Bulgin/PX0794-S?qs=fZ%2FLqxhLDmlcL7BQN2egOg%3D%3D">Mouser</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">PX0794/P</td>
<td style="text-align: center;">Bulgin PX0794 Circular Connector, Plug</td>
<td style="text-align: center;"><a href="https://www.mouser.co.uk/ProductDetail/Bulgin/PX0794-P?qs=1heAGhfc8RDP1j2J1H7tmQ%3D%3D">Mouser</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">PV0H24011-311</td>
<td style="text-align: center;">Panel Mount Illuminated Pushbutton Switch</td>
<td style="text-align: center;"><a href="https://www.mouser.co.uk/ProductDetail/E-Switch/PV0H24011-311?qs=QvRObDHRS1%252BbpJ%252BDa6XY%2Fg%3D%3D">Mouser</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">PX0708/P/12</td>
<td style="text-align: center;">Bulgin PX0708 12-way Circular Connector, Plug</td>
<td style="text-align: center;"><a href="https://www.bulgin.com/en/search#q=PX0708">Bulgin Search</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">PX0708/S/12</td>
<td style="text-align: center;">Bulgin PX0708 12-way Circular Connector, Socket</td>
<td style="text-align: center;"><a href="https://www.bulgin.com/en/search#q=PX0708">Bulgin Search</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">177-PX0709/P/07</td>
<td style="text-align: center;">Bulgin PX0709 7-way Circular Connector, Plug</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=PX0709%2FP%2F07">Mouser Search</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/bulgin-px0709-7-way-plug.png" style="width:1.31268in;height:1.18767in" /></td>
</tr>
<tr>
<td style="text-align: center;">167-PX0709/S/07</td>
<td style="text-align: center;">Bulgin PX0709 7-way Circular Connector, Socket</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=PX0709%2FS%2F07">Mouser Search</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/bulgin-px0709-7-way-socket.png" style="width:1.41096in;height:1.25892in" /></td>
</tr>
<tr>
<td style="text-align: center;">167-PX0745/P/07</td>
<td style="text-align: center;">Bulgin PX0745 7-way Circular Connector, Plug</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=PX0745%2FP%2F07">Mouser Search</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/bulgin-px0745-7-way-plug.png" style="width:1.57314in;height:1.07307in" /></td>
</tr>
<tr>
<td style="text-align: center;">167-PX0745/S</td>
<td style="text-align: center;">Bulgin PX0745 Circular Connector, Socket</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=PX0745%2FS">Mouser Search</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">157-SA3348/1</td>
<td style="text-align: center;">Bulgin SA3348 Connector Accessory / Back-Shell</td>
<td style="text-align: center;"><a href="https://uk.rs-online.com/web/c/?searchTerm=SA3348%2F1">RS Components</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">157-SA3347/1</td>
<td style="text-align: center;">Bulgin SA3347 Connector Accessory / Back-Shell</td>
<td style="text-align: center;"><a href="https://uk.rs-online.com/web/c/?searchTerm=SA3347%2F1">RS Components</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">157-13027/1</td>
<td style="text-align: center;">Bulgin 13027 Connector Accessory</td>
<td style="text-align: center;"><a href="https://uk.rs-online.com/web/c/?searchTerm=13027%2F1">RS Components</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">62D11-02-060C</td>
<td style="text-align: center;">Grayhill Premium Haptic Optical Rotary Encoder, 32 PPR, w/ Pushbutton</td>
<td style="text-align: center;"><a href="https://www.grayhill.com/products/62d11-02-060c">Grayhill</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/grayhill-62d11-rotary-encoder.jpeg" style="width:2.82192in;height:2.82192in" alt="62D11-02-060C" /></td>
</tr>
<tr>
<td style="text-align: center;">06SR-3S</td>
<td style="text-align: center;">JST/Molex 3-pin Encoder Cable Connector (Plug)</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=06SR-3S">Mouser Search</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">#1D</td>
<td style="text-align: center;">IAN CANADA — FifoPi Q7 Synchronous FIFO Reclocker Board, I2S 32-bit 768kHz DSD1024 DoP256</td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/1d-fifopi-q7-flagship-i2s-dsd-dop-fifo-with-isolator-re-clocker-and-low-phase-noise-xos">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-fifopi-q7.jpeg" style="width:2.86395in;height:1.90919in" alt="FifoPi Q7 III" /></td>
</tr>
<tr>
<td style="text-align: center;">#38A</td>
<td style="text-align: center;">IAN CANADA — LinearPi Dual Ultra-Low Noise Linear Power Supply Module 2x +/-5V / +/-3.3V 2.5A</td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/linearpi-pro-high-current-ultra-low-noise-linear-power-supply-full-smt">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-linearpi-dual.jpeg" style="width:2.50939in;height:1.58219in" alt="LinearPi Dual" /></td>
</tr>
<tr>
<td style="text-align: center;">#35B</td>
<td style="text-align: center;">IAN CANADA — StationPi PRO Raspberry Pi and HAT Boards Adapter Station</td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/35b-stationpi-pro-fully-finished">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-stationpi-pro.jpeg" style="width:2.50699in;height:1.67123in" alt="StationPi Pro" /></td>
</tr>
<tr>
<td style="text-align: center;">CCHD-957-45</td>
<td style="text-align: center;">CRYSTEK CCHD-957 Ultra Low Phase Noise Clock, 45.1584 MHz, 3.3 V, 25 ppm</td>
<td style="text-align: center;"><a href="https://www.crystek.com/crystal/spec-sheets/clock/CCHD-957.pdf">Crystek Datasheet</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/crystek-cchd957-clock.png" style="width:1.35436in;height:1.30226in" /></td>
</tr>
<tr>
<td style="text-align: center;">CCHD-957-49</td>
<td style="text-align: center;">CRYSTEK CCHD-957 Ultra Low Phase Noise Clock, 49.152 MHz, 3.3 V, 25 ppm</td>
<td style="text-align: center;"><a href="https://www.crystek.com/crystal/spec-sheets/clock/CCHD-957.pdf">Crystek Datasheet</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/crystek-cchd957-clock.png" style="width:1.35436in;height:1.30226in" /></td>
</tr>
<tr>
<td style="text-align: center;">CCHD957-Adapter</td>
<td style="text-align: center;">IAN CANADA — CCHD957 XO Clock Adapter, supports SMT Capacitors (pair)</td>
<td style="text-align: center;"><a href="https://iancanada.ca/">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-cchd957-clock-adapter.png" style="width:2.43836in;height:1.49056in" /></td>
</tr>
<tr>
<td style="text-align: center;">#32B</td>
<td style="text-align: center;">IAN CANADA — LinearPi MKII SOLO Ultra-Low Noise Linear Power Supply Module, 5V / 3.3V / 12V, 2.5A</td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/linearpi-mkii-ultra-low-noise-smt-linear-power-supply">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-linearpi-mkii-solo.jpeg" style="width:2.46589in;height:1.64384in" alt="LinearPi MkII Solo" /></td>
</tr>
<tr>
<td style="text-align: center;">AS318-B-451584</td>
<td style="text-align: center;">ACCUSILICON AS318-B Ultra Low Jitter Audio Clock, 45.1584 MHz</td>
<td style="text-align: center;"><a href="https://www.accusilicon.com/">AccuSilicon</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">AS318-B-491520</td>
<td style="text-align: center;">ACCUSILICON AS318-B Ultra Low Jitter Audio Clock, 49.152 MHz</td>
<td style="text-align: center;"><a href="https://www.accusilicon.com/">AccuSilicon</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">#49B</td>
<td style="text-align: center;">IAN CANADA — MonitorPi PRO Control Center and Signal Analyzer with Display for Raspberry Pi</td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/49b-monitorpi-pro-integrated-control-center-and-signal-analyzer">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-monitorpi-pro.jpeg" style="width:2.48644in;height:1.65753in" alt="MonitorPi Pro" /></td>
</tr>
<tr>
<td style="text-align: center;">#19C</td>
<td style="text-align: center;">IAN CANADA — ReceiverPi PRO II Interface, SPDIF / I2S / HDMI for Raspberry Pi</td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/19c-receiverpi-pro-ii">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-receiverpi-pro-ii.jpeg" style="width:2.47616in;height:1.65068in" alt="ReceiverPi Pro II" /></td>
</tr>
<tr>
<td style="text-align: center;">UcCond-MKII-3V3</td>
<td style="text-align: center;">IAN CANADA — UcConditioner MKII Ultra Capacitor Conditioner Board, 3.3V</td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/ucconditionermkii-5v-or-3-3v">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/ucconditioner-mkii-product-thumbnail.png" style="width:1.36652in;height:1.36652in" /></td>
</tr>
<tr>
<td style="text-align: center;">UcCond-MKII-5v</td>
<td style="text-align: center;">IAN CANADA — UcConditioner MKII Ultra Capacitor Conditioner Board, 5V</td>
<td style="text-align: center;"></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/ucconditioner-mkii-product-thumbnail.png" style="width:1.15385in;height:1.15385in" /></td>
</tr>
<tr>
<td style="text-align: center;">UCPure Balancer</td>
<td style="text-align: center;"><strong>UcBalancer protection board KIT</strong></td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/41f-ucbalancer-protection-board-kit">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-ucbalancer-protection-board.png" style="width:2.03611in;height:1.49315in" /></td>
</tr>
<tr>
<td style="text-align: center;">Alu-Case-320x240x90</td>
<td style="text-align: center;">100% Aluminium DIY Box / Case, Round Corners, 320×240×90 mm, Black</td>
<td style="text-align: center;"><a href="https://www.aliexpress.com/wholesale?SearchText=aluminium+case+320x240x90">Search AliExpress</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">#50A</td>
<td style="text-align: center;">IAN CANADA — GPIO 40-PIN Extension Kit for Raspberry Pi (with 6″ &amp; 12″ FFC cables)</td>
<td style="text-align: center;"><a href="https://iancanada.ca/products/50a-universal-raspberrypi-gpio-extension-kit-with-6-12-ffc-cables">Ian Canada</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/iancanada-gpio-extension-kit.jpeg" style="width:2.72603in;height:1.81725in" alt="GPIO Extension Kit" /></td>
</tr>
<tr>
<td style="text-align: center;"></td>
<td style="text-align: center;"></td>
<td style="text-align: center;"></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">D-Shaft-Button-40mm</td>
<td style="text-align: center;">Aluminum Knob — D-Shaft, 40 mm diameter, Ø6 mm bore, Black</td>
<td style="text-align: center;"><a href="https://www.aliexpress.com/wholesale?SearchText=aluminum+knob+d+shaft+40mm+6mm">Search AliExpress</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">BTR-M3x5</td>
<td style="text-align: center;">Socket Head Cap Screw, Steel, M3×5 mm (Pack of 10)</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=M3+5mm+socket+head">Search</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">LHY-LT3042-2x5V</td>
<td style="text-align: center;">LHY AUDIO — Dual Linear Power Supply Module LT3042, 2×5V, 1.5A</td>
<td style="text-align: center;"><a href="https://www.audiophonics.fr/en/regulated-psu/lhy-audio-dual-linear-power-supply-module-lt3042-2x5v-15a-p-17276.html">audiophonics</a></td>
<td style="text-align: center;"><img src="../../user-manual-media/media/service-guide/lhy-lt3042-dual-5v-supply.png" style="width:1.76309in;height:1.76712in" /></td>
</tr>
<tr>
<td style="text-align: center;">MicroUSB-Bare-20cm</td>
<td style="text-align: center;">Micro USB Male to Bare Wire Power Cable, 24AWG, 20 cm</td>
<td style="text-align: center;"><a href="https://www.aliexpress.com/wholesale?SearchText=micro+usb+male+bare+wire+20cm">Search AliExpress</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">SE-BAL-XLR-RCA</td>
<td style="text-align: center;">Single-Ended to Balanced Converter Module, XLR / RCA Stereo</td>
<td style="text-align: center;"><a href="https://www.aliexpress.com/wholesale?SearchText=single+ended+balanced+converter+XLR+RCA">Search AliExpress</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">Toroid-Cover-88x62</td>
<td style="text-align: center;">Toroid Cover — Metal Iron Shield for Transformer, 88×62 mm</td>
<td style="text-align: center;"><a href="https://www.aliexpress.com/wholesale?SearchText=toroid+cover+transformer+shield+88mm">Search AliExpress</a></td>
<td style="text-align: center;"></td>
</tr>
<tr>
<td style="text-align: center;">Toroidal-15VA-2x12V</td>
<td style="text-align: center;">Toroidal Transformer, 15VA, 2×12V AC</td>
<td style="text-align: center;"><a href="https://www.mouser.com/Search/Refine?Keyword=toroidal+transformer+15VA+12V">Search</a></td>
<td style="text-align: center;"></td>
</tr>
</tbody>
</table>

### Power Distribution — To Do

*Diagram showing every rail.*

### Transformer Connections — To Do

*Photos plus wiring diagrams.*

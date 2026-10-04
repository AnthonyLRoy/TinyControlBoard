# Streamer DAC User Manual

Use this manual to set up and operate the Streamer DAC and its external power supply. Follow Quick Start for the initial connections, then refer to the relevant sections for everyday use or troubleshooting.

## Quick Start

1. Make sure the power supply's rear-panel switch is **OFF**.
2. Connect the DAC to the external power supply. Connect the earth and control leads, then connect the keyed DC power leads. Use the rear-panel diagrams to identify each connection. Never force a plug.
3. Connect a digital audio source to a compatible input. The I²S input uses an HDMI-style socket, but it is **not** an HDMI connection. Read the I²S input description before connecting a source.
4. Connect the DAC's RCA or XLR outputs to an amplifier or pre-amplifier.
5. Connect Ethernet if you need network features. Connect other optional sources or USB storage as required.
6. Connect the mains lead and switch on the power supply.
7. Briefly press the power button beneath the DAC's front edge. Wait for startup to finish. The expected indicators are described in [Startup and Troubleshooting](#startup-and-troubleshooting).

![User manual figure 1](../user-manual-media/media/androidApp/image1.png)

## Everyday Operation

### Front Panel at a Glance

- **Buttons 1–6** control the main display and its presentation.
- **Buttons 7–12** control playback.
- **Mini display [15]** and its adjacent controls [14] and [16] provide supplementary DAC information and controls. For details on using the mini display and knob [14], see Ian Canada's [MonitorPi Reference manual](https://github.com/iancanada/DocumentDownload/blob/master/MonitorPi/MonitorPiPro/MonitorPiProManual.pdf).
- **Main display [19]** shows album artwork, track information and menus.
- **Rotary knob [17]** controls track selection and playback.
- **Power indicator [18]** shows the player's power state.
- **Power button [13]** is beneath the front edge of the cabinet.

### Power, Standby and Deep Sleep

Briefly press the power button [13] to wake the player or put it into standby. In standby, the main display and output stage switch off, but the DAC remains powered so the player can wake quickly.

To enter Deep Sleep, press and hold the power button for about **3 seconds**, then release it. The player shuts down before entering Deep Sleep, which switches off both the DAC and output stage. Briefly press the power button to wake the player. If you will not use it for several days, switch off the external power supply.

### Playback Controls

Use the six buttons on the right side of the panel to control playback:

- **Previous Track [7]** and **Next Track [9]** move to the previous or next track.
- **Repeat [8]** repeats the playlist. The button stays lit while repeat is on; press it again to turn repeat off.
- **Skip Backwards [10]** and **Skip Forward [12]** move playback by 10 seconds with each press.
- **Random [11]** turns shuffle on or off. The button stays lit while shuffle is on.

The rotary knob [17] also controls playback: turn it right for the next track, turn it left for the previous track, and press it to play or pause.

### Display Controls

Use the six buttons on the left side of the panel to control the main display:

- **Display over Art [1]** shows or hides track information over album artwork. The button stays lit while the overlay is visible.
- **Screen Brightness [2]** cycles through display brightness levels. The button lighting is synchronised with screen brightness.
- **Meter Display [3]** shows or hides the audio level meter. The button stays lit while the meter is active.
- **Toggle DAC [4]** switches between the DAC outputs. The button stays lit when the alternate output is selected.
- **Switch Display [5]** turns the main display off or on without interrupting music playback.
- **Switch Display Panel [6]** cycles through the available screen views.

The other buttons briefly light to confirm a press. For the mini display and its controls, see the linked MonitorPi manual in [Front Panel at a Glance](#front-panel-at-a-glance).

### Front-Panel Quick Reference

| Ref (fig. 1) | Control | Function |
|---|---|---|
| 1 | Display over Art | Shows or hides the interface overlay on album artwork; lit when active. |
| 2 | Screen Brightness | Cycles through display brightness levels. |
| 3 | Meter Display | Shows or hides the audio level meter; lit when active. |
| 4 | Toggle DAC | Switches DAC output; lit when the alternate output is selected. |
| 5 | Switch Display | Turns the main display off or on. |
| 6 | Switch Display Panel | Cycles through screen views. |
| 7 | Previous Track | Moves to the previous track. |
| 8 | Repeat | Toggles playlist repeat; lit when active. |
| 9 | Next Track | Moves to the next track. |
| 10 | Skip Backwards | Moves playback back by 10 seconds. |
| 11 | Random | Toggles shuffle; lit when active. |
| 12 | Skip Forward | Moves playback forward by 10 seconds. |
| 13 | Power button | Brief press for standby/wake; hold about 3 seconds for Deep Sleep. |
| 17 | Rotary knob | Turn to change tracks; press to play or pause. |
| 18 | Power indicator | Indicates the player's power state. |
| 19 | Main display | Shows artwork, track information and menus. |

## Connecting Equipment

### DAC Rear Panel

![User manual figure 2](../user-manual-media/media/androidApp/image2.png)

The rear panel provides power and control connections, digital audio inputs, network and USB connections, and analogue outputs. Most setups only require the power connections, one audio input and one pair of outputs.

#### Power and Ground Connections

**1 & 3 – Chassis earth connections**

These banana sockets connect the DAC chassis to the earth connection on the external power supply. Connecting the chassis earth to the power supply is recommended. In most installations, no further grounding adjustment is needed.

**2 – Power-supply control connection**

Connects the DAC to the external power supply and carries the control signals used for startup, standby and Deep Sleep.

**4 & 5 – DC power inputs**

These keyed connectors carry DC power from the external supply. Align each connector before inserting it. Do not force it; the locking mechanism secures a correctly aligned connector.

#### Digital Audio Inputs

**8 – I²S digital audio input (HDMI-style connector)**

This socket carries I²S digital audio, not HDMI video or audio. External I²S pinouts are not standardised between manufacturers. This input is designed for sources using the **PS Audio I²S pinout**; check compatibility before connecting a source.

**9 – USB Audio input**

This USB Type-B input connects compatible computers and other supported USB Audio sources. Playback capability depends on the source operating system and software.

**12 – Optical (TOSLINK) input**

Connect a digital audio source with an optical output, such as a television, media player, game console or CD player.

**13 – Coaxial S/PDIF input**

Connect a digital audio source with a coaxial S/PDIF output, such as a CD transport, streamer or digital audio interface.

#### Network, USB and Expansion

**6 – Firmware update port**

Use this port only for firmware updates. It is not intended for music playback or USB storage.

**7 – USB storage port**

Connect compatible USB storage, such as a flash drive or externally powered hard drive, then select the device in the DAC's user interface.

**10 – Expansion port**

This port is reserved for future expansion and is not currently active. Leave the supplied blanking plate in place.

**11 – Ethernet network port**

Use a standard Ethernet cable to connect the DAC to your home network. This enables supported streaming, remote-control, and firmware-notification features. A wired connection is recommended for reliable streaming.

#### Analogue Outputs

**14 & 15 – RCA analogue outputs**

- **14** – Right channel
- **15** – Left channel

Connect these single-ended outputs to RCA inputs on an amplifier or pre-amplifier.

**16 & 17 – Balanced XLR outputs**

- **16** – Right channel
- **17** – Left channel

Connect these balanced outputs to XLR inputs on an amplifier or pre-amplifier. Balanced connections can help reject electrical interference, particularly with longer cable runs.

#### Ground-Lift Switches

Switches **18 (RCA)** and **19 (XLR)** are marked **Not Implemented**. Their described ground-lift function is not available; do not use them as a hum-reduction adjustment.

### DAC Rear-Panel Quick Reference

| Ref (fig. 2) | Connection | Purpose |
|---|---|---|
| 1 & 3 | Chassis earth | Connects the DAC chassis to the power-supply earth. |
| 2 | Power-supply control | Control link between the DAC and external power supply. |
| 4 & 5 | DC power inputs | DC power from the external supply. |
| 6 | Firmware update port | Reserved for firmware updates. |
| 7 | USB storage | Connects compatible USB storage. |
| 8 | I²S audio input | I²S digital audio using a compatible PS Audio pinout; not HDMI. |
| 9 | USB Audio input | USB Type-B digital audio input. |
| 10 | Expansion port | Reserved; keep the blanking plate fitted. |
| 11 | Ethernet | Network connection. |
| 12 | Optical input | TOSLINK digital audio input. |
| 13 | Coaxial input | Coaxial S/PDIF digital audio input. |
| 14 & 15 | RCA outputs | Right and left single-ended analogue outputs. |
| 16 & 17 | XLR outputs | Right and left balanced analogue outputs. |
| 18 & 19 | Ground-lift switches | Not implemented. |

### External Power Supply

#### Front Panel

![User manual figure 3](../user-manual-media/media/androidApp/image3.png)

The three LEDs on the power supply's front panel indicate its status:

- **Power indicator [1]** – Indicates that the unit is receiving mains power.
- **5V UcPure [2]** – Orange while its capacitors charge; green when fully charged and supplying power from the capacitors.
- **3.3V UcPure [3]** – Orange while its capacitors charge; green when fully charged and supplying power from the capacitors.

![User manual figure 4](../user-manual-media/media/androidApp/image4.png)

#### Back Panel

**1 & 3 – Earth connections**

These banana sockets connect the power-supply earth to the DAC chassis. Connecting one of these to the DAC is recommended.

**2 – Power-supply control connection**

Connects the external power supply to the DAC's control connection. It enables coordinated startup, standby and Deep Sleep operation.

**9 & 10 – Main power connections**

These keyed connectors supply DC power to the DAC. Match each connector to its corresponding socket and never force it into place.

**4–7 – Output fuses**

These fuses protect the power-supply outputs and should not normally need replacing. If a fuse blows, identify and correct the cause before replacing it. A fuse that blows repeatedly may indicate a fault in the connected equipment or wiring.

> **Warning:** Switch the power supply off and disconnect the mains lead before inspecting or replacing a fuse. Replace a fuse only with the same type and current rating specified for that position. If you are unsure of the specification or replacement procedure, contact a qualified service technician.

**8 – Mains inlet, fuse and switch**

The IEC mains inlet includes the power switch and an integrated fuse. Before connecting the mains lead, make sure the switch is **OFF** and the mains voltage is compatible with the unit.

> **Warning:** Disconnect the mains lead before replacing the mains fuse. Use only the specified fuse type and rating.

## Startup and Troubleshooting

### Normal Startup

When you press the power button to wake the player, the indicators should behave as follows:

1. The power indicator [18] flashes red.
2. Button LEDs [1, 2, 3, 4, 5, 6, 7 and 10] illuminate blue.
3. The button LEDs turn off as startup proceeds.
4. When startup is complete, the button LEDs are off and the power indicator is solid blue. The DAC is ready to use.

### Startup Error Indicators

If the player does not complete startup, the following flashing button combinations indicate a fault:

| Flashing buttons | Indicated issue |
|---|---|
| 7 and 10 | Display screen |
| 3 and 6 | DAC section |
| 2 and 5 | Clocks or DAC power |
| 1 and 4 | Power to the streamer |
| All buttons | Control system board |

Check that the DAC's power and control connections to the power supply are secure. Switch the system off, wait a couple of minutes, and try again. If the fault persists, stop using the unit and contact the supplier or a qualified service technician. Do not attempt internal repairs.

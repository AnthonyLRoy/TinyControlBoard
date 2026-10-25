# Case Assembly Guide

[Back to the Service Guide](../ServiceGuide.md) · [Mechanical construction](MechanicalConstruction.md) · [System wiring and commissioning](SystemWiring.md)

| Document       | Value                       |
| -------------- | --------------------------- |
| Revision       | 0.1 (draft)                 |
| Date           | YYYY-MM-DD                  |
| Applies to     | DAC Case, Power Supply Case |
| Skill level    | DIY / semi-professional     |
| Estimated time | Front panel: _TBD_ h      |

> **Drawing convention:** every `📐 DRAWING` marker below is a placeholder. Replace it with the image link, keeping the figure number and caption. Store images in `docs/user-manual-media/media/case-assembly/`.

## Contents

1. [Introduction](#1-introduction)
2. [Safety](#2-safety)
3. [Tools and Materials](#3-tools-and-materials)
4. [Before You Start](#4-before-you-start)
5. [DAC Case](#5-dac-case)
   - [5.1 Case Overview](#51-case-overview)
   - [5.2 Parts List](#52-parts-list)
   - [5.3 Front Panel](#53-front-panel)
   - [5.4 Rear Panel](#54-rear-panel-to-do) *(to do)*
   - [5.5 Side Panels](#55-side-panels-to-do) *(to do)*
   - [5.6 Top Panel](#56-top-panel-to-do) *(to do)*
   - [5.7 Bottom Panel](#57-bottom-panel-to-do) *(to do)*
   - [5.8 Final Case Assembly](#58-final-case-assembly-to-do) *(to do)*
6. [Power Supply Case](#6-power-supply-case-to-do) *(to do)*
7. [Final Inspection and Checklist](#7-final-inspection-and-checklist)
8. [Troubleshooting](#8-troubleshooting)
9. [Maintenance and Care](#9-maintenance-and-care)
10. [Revision History](#10-revision-history)

---

## 1. Introduction

This guide describes how to assemble the enclosures for the streamer/DAC system. There are two cases:

| Case                        | Purpose                               | Panels                                          |
| --------------------------- | ------------------------------------- | ----------------------------------------------- |
| **DAC Case**          | Streamer, DAC and control electronics | Top, bottom, front, rear, 2 × side (identical) |
| **Power Supply Case** | Power supply and capacitor bank       | _TBD_                                         |

This revision covers the **DAC Case front panel** only. Remaining panels will be added in later revisions.

### 1.1 How to use this guide

- Read the whole section for a panel before starting it.
- Steps are numbered and must be completed in order.
- Dimensions are in millimetres unless stated otherwise.
- Callouts like **(A)** refer to labels in the drawing for that step.

### 1.2 Related documents

- [Mechanical construction](MechanicalConstruction.md) – enclosure drawings and bill of materials
- [System wiring and commissioning](SystemWiring.md)
- [Power supply subsystem](PowerSupplySubsystem.md)

---

## 2. Safety

> ⚠️ **WARNING – Mains and stored energy.** The power supply case contains mains voltage and supercapacitors that retain charge after power-off. Never work on a powered unit. See the safety section of the [user manual](../user-manual.md).

- Disconnect from the mains and wait for capacitors to discharge before opening any case.
- Wear eye protection when drilling, cutting or filing aluminium.
- Deburr all edges; panel edges can cut.
- Support panels on a soft surface to avoid scratching the finish.
- Avoid over-tightening screws into aluminium; stripped threads cannot be repaired easily.

---

## 3. Tools and Materials

### 3.1 Tools

| Tool                          | Size / spec  | Used for                  |
| ----------------------------- | ------------ | ------------------------- |
| Hex (Allen) keys              | _TBD_      | Panel screws              |
| Screwdrivers                  | _TBD_      | _TBD_                   |
| Spanner / nut driver          | _TBD_      | Connector and switch nuts |
| Calipers                      | 0.02 mm      | Checking cut-outs         |
| Torque screwdriver (optional) | _TBD_ N·m | Final tightening          |
| Soft cloth / mat              | –           | Surface protection        |
| Isopropyl alcohol             | –           | Cleaning panels           |
| Anti-static wrist strap       | –           | Handling boards           |

### 3.2 Consumables

- Thread-locking compound (low strength), if specified
- Lint-free wipes
- Cable ties / adhesive pads, if specified

---

## 4. Before You Start

### 4.1 Unpacking and inspection

1. Unpack all panels and check against the [parts list](#52-parts-list).
2. Inspect for scratches, dents, burrs, and misaligned or missing holes.
3. Check cut-out sizes against the dimensioned drawings in this guide.
4. Report or set aside any damaged parts before assembly.

### 4.2 Workspace

- Clean, flat, well-lit bench with a soft mat.
- Small parts tray for screws and nuts.

### 4.3 Conventions

| Term         | Meaning                                                  |
| ------------ | -------------------------------------------------------- |
| Front / Rear | As seen when facing the unit from the listening position |
| Left / Right | As seen from the front                                   |
| Outer face   | The face visible after assembly                          |
| Inner face   | The face towards the electronics                         |

---

## 5. DwordAC Case

### 5.1 Case Overview

The DAC case consists of six panels. The two side panels are identical.

| Panel  | Qty           | Section               |
| ------ | ------------- | --------------------- |
| Front  | 1             | [5.3](#53-front-panel) |
| Rear   | 1             | 5.4                   |
| Side   | 2 (identical) | 5.5                   |
| Top    | 1             | 5.6                   |
| Bottom | 1             | 5.7                   |

📐 **DRAWING – Figure 5.1:** Complete case isometric view
`![Complete case isometric view](../../user-manual-media/media/case-assembly/fig-5-1-complete-case-iso.png)`

📐 **DRAWING – Figure 5.2:** Exploded isometric view of the assembled case
`![Exploded isometric view](../../user-manual-media/media/case-assembly/fig-5-2-exploded-iso.png)`

**Overall dimensions:** W × D × H = _TBD_ × _TBD_ × _TBD_ mm

### 5.2 Parts List

#### Case panels and hardware

| Item | Description          | Qty     | Notes     |
| ---- | -------------------- | ------- | --------- |
| 1    | Front panel          | 1       |           |
| 2    | Rear panel           | 1       |           |
| 3    | Side panel           | 2       | Identical |
| 4    | Top panel            | 1       |           |
| 5    | Bottom panel         | 1       |           |
| 6    | Panel screws _TBD_ | _TBD_ |           |
| 7    | _TBD_              |         |           |

#### Front panel components

| Item | Description             | Qty     | Notes                                                               |
| ---- | ----------------------- | ------- | ------------------------------------------------------------------- |
| F1   | Control pushbuttons     | _TBD_ | See [BOM](MechanicalConstruction.md#main-assembly-bill-of-materials) |
| F2   | Display (MonitorPi Pro) | 1       |                                                                     |
| F3   | Knob(s)                 | _TBD_ |                                                                     |
| F4   | LEDs / LED holders      | _TBD_ |                                                                     |
| F5   | Fixing screws _TBD_   | _TBD_ |                                                                     |

Confirm all part numbers against the [bill of materials](MechanicalConstruction.md#main-assembly-bill-of-materials).

---

### 5.3 Front Panel

#### 5.3.1 Overview

_One or two sentences: what the front panel carries (controls, display, indicators) and how it connects to the rest of the case._

**Material / finish:** _TBD_  **Thickness:** _TBD_ mm  **Mass:** _TBD_ g

#### 5.3.2 Drawings

📐 **DRAWING – Figure 5.3:** Front panel, front view, with measurements
`![Front panel front view](../../user-manual-media/media/case-assembly/fig-5-3-front-panel-front.png)`

📐 **DRAWING – Figure 5.4:** Front panel, back view, with size measurements
`![Front panel back view](../../user-manual-media/media/case-assembly/fig-5-4-front-panel-back.png)`

📐 **DRAWING – Figure 5.5:** Front panel, front isometric view, with measurements
`![Front panel front isometric](../../user-manual-media/media/case-assembly/fig-5-5-front-panel-front-iso.png)`

📐 **DRAWING – Figure 5.6:** Front panel, back isometric view, with measurements
`![Front panel back isometric](../../user-manual-media/media/case-assembly/fig-5-6-front-panel-back-iso.png)`

#### 5.3.3 Cut-out and hole schedule

| Ref | Feature           | Size (mm) | Position X, Y (mm) | Fitted component |
| --- | ----------------- | --------- | ------------------ | ---------------- |
| A   | Display window    | _TBD_   | _TBD_            | Display          |
| B   | Button hole       | _TBD_   | _TBD_            | Pushbutton       |
| C   | Knob / shaft hole | _TBD_   | _TBD_            | Knob             |
| D   | LED hole          | _TBD_   | _TBD_            | LED              |
| E   | Mounting hole     | _TBD_   | _TBD_            | Case screw       |

Tolerance: ± _TBD_ mm unless stated otherwise.

#### 5.3.4 Preparation

1. Place the panel face-down on a soft mat.
2. Check every hole against the schedule above using calipers.
3. Deburr and clean the holes with isopropyl alcohol.
4. Dry-fit each component before final fitting.

#### 5.3.5 Assembly steps

> Replace the steps below with the final sequence once confirmed on the real build.

**Step 1 – Fit the display**

_Instruction text._

📐 **DRAWING / PHOTO – Figure 5.7:** Display fitting

**Step 2 – Fit the pushbuttons**

_Instruction text._

📐 **DRAWING / PHOTO – Figure 5.8:** Pushbutton fitting

**Step 3 – Fit the LEDs**

_Instruction text._

**Step 4 – Fit the knob(s)**

_Instruction text._

**Step 5 – Connect cabling**

_Instruction text. Include connector references from the [system wiring](SystemWiring.md) guide._

**Step 6 – Check**

- [ ] All components sit flush and square
- [ ] Nothing is loose
- [ ] No scratches on the outer face
- [ ] Buttons move freely and return fully

#### 5.3.6 Fixing the front panel to the case

_To be completed when the remaining panels are documented. Reference the exploded view (Figure 5.2)._

#### 5.3.7 Torque and fastener reference

| Fastener | Used for    | Qty     | Torque       |
| -------- | ----------- | ------- | ------------ |
| _TBD_  | Case fixing | _TBD_ | _TBD_ N·m |

#### 5.3.8 Notes and common mistakes

- _e.g. panel fitted upside down – check orientation against Figure 5.3._
- _e.g. over-tightening shaft nuts marks the finish – use a protective washer._

---

### 5.4 Rear Panel *(to do)*

📐 **DRAWING – Figure 5.9:** Rear panel with size measurements
📐 **DRAWING – Figure 5.10:** Rear panel isometric view

### 5.5 Side Panels *(to do)*

The two side panels are identical.

📐 **DRAWING – Figure 5.11:** Side panel front view with measurements

### 5.6 Top Panel *(to do)*

📐 **DRAWING – Figure 5.12:** Top view with measurements

### 5.7 Bottom Panel *(to do)*

📐 **DRAWING – Figure 5.13:** Bottom panel view with measurements

### 5.8 Final Case Assembly *(to do)*

_Panel order, fastening sequence, and final torque._

---

## 6. Power Supply Case *(to do)*

_Same structure as section 5: overview, parts list, one subsection per panel, final assembly._

---

## 7. Final Inspection and Checklist

- [ ] All panels fitted, aligned and flush
- [ ] All fasteners installed and tightened to specification
- [ ] No visible scratches, dents or loose parts
- [ ] Controls operate smoothly
- [ ] Cables routed clear of edges and moving parts
- [ ] Earth/ground bonding verified (continuity test), if applicable
- [ ] Case labelled as required

## 8. Troubleshooting

| Problem              | Likely cause                               | Remedy                                                   |
| -------------------- | ------------------------------------------ | -------------------------------------------------------- |
| Panel does not align | Fastener loosely started or panel reversed | Loosen all fasteners, re-seat panel, tighten in sequence |
| Button sticks        | Hole burr or over-tightened nut            | Deburr, loosen nut                                       |
| Display not flush    | Spacer missing or wrongly fitted           | Check against Figure 5.7                                 |
| _TBD_              |                                            |                                                          |

## 9. Maintenance and Care

- Clean aluminium with a soft, slightly damp cloth; avoid abrasive cleaners and solvents.
- Check fasteners periodically for loosening.

## 10. Revision History

| Rev | Date       | Author  | Changes                           |
| --- | ---------- | ------- | --------------------------------- |
| 0.1 | YYYY-MM-DD | _TBD_ | Initial template, DAC front panel |

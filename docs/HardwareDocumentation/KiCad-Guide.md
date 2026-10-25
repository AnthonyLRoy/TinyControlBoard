# KiCad Guide

This guide describes how to work with and maintain the project's custom PCB designs in
KiCad. It is a workflow guide, not a circuit specification or substitute for the board
schematics.

## Current repository status

There are currently no KiCad project, schematic, board-layout, or library files in this
repository. Consequently, this guide does not document specific board revisions, component
values, connector pinouts, or fabrication settings. Do not use firmware pin assignments or
the wiring reference as a substitute for a reviewed schematic.

When a board project is added, keep its KiCad source files together under
`docs/HardwareDocumentation/PCBs/<board-name>/`. Keep board-specific manufacturing exports
in a clearly named release directory, separate from the editable source.

## Prerequisites

- Install KiCad using the official installer for your operating system.
- Use the same KiCad major version as the other contributors when practical. Record the
  version used to validate a board release in that release's notes.
- Before editing, make a backup or use a Git branch. KiCad projects contain multiple
  related files; commit a coherent project change together.

## Open and edit a project

1. Open the board's `.kicad_pro` file in KiCad. Open the `.kicad_sch` or `.kicad_pcb`
   directly only when working on that document.
2. Review the project hierarchy and any project-specific notes before editing.
3. Make schematic changes first when the circuit changes. Annotate and update the PCB from
   the schematic so the two remain synchronized.
4. If a component, connector, or footprint changes, check the physical fit and the related
   wiring and enclosure documentation as well as the firmware interface.
5. Save all project documents, then run the checks below before sharing or releasing the
   board.

## Libraries and project files

- Keep custom symbols and footprints needed by the design in the board project's own
  library directories, with the corresponding KiCad library table entries.
- Prefer standard KiCad libraries for unmodified parts. Record the source and version for
  externally sourced custom library items, and review their licensing before including
  them.
- Commit editable project files and any custom libraries required to open the project on a
  clean machine.
- Do not commit machine-specific absolute library paths, temporary lock files, autosaves,
  backups, or caches. Verify that a fresh checkout can open the project without relying on
  files outside the repository.

## Design checks

Before considering a revision ready:

1. Run Electrical Rules Checker (ERC) on the schematic.
2. Synchronize the PCB with the schematic and confirm that no intended connections or
   components are missing.
3. Run Design Rules Checker (DRC) on the PCB using the actual board constraints.
4. Resolve reported issues, or document why each remaining warning is intentional. Do not
   suppress a rule simply to make the report clean.
5. Visually inspect the schematic, board outline, copper zones, silkscreen, connector
   orientation, and clearances. Confirm that the footprints match the physical parts.

Store a dated ERC/DRC report with a fabrication release when practical. The reports do not
replace review of the design or verification of the manufacturer's capabilities.

## Fabrication release

Create fabrication files from the reviewed board revision, not from a working copy with
uncommitted changes. Use the board manufacturer's current requirements and verify the
generated outputs before sending them out.

A release should identify the board name and revision and include only the deliverables
required for that order, typically:

- Gerber copper, solder-mask, silkscreen, and board-outline layers;
- drill files;
- assembly outputs such as BOM and component-position files when assembly is requested;
- relevant ERC/DRC reports and a short release note.

Reopen or inspect the generated files in a Gerber viewer and verify the layer set, board
outline, drill data, and revision. KiCad export defaults are not a substitute for checking
the fabricator's specifications. Keep released outputs distinguishable from editable
sources; do not overwrite a previous revision's release.

## Keeping project documentation consistent

When a PCB change affects a physical or software interface, update the relevant project
documentation in the same change:

- [Hardware Interface Reference](../DeveloperGuides/06-hardware-interface.md) for firmware
  GPIO, buses, relays, and connector interfaces;
- [Wiring Diagrams and Bill of Materials](../README.md#hardware-documentation) for harness
  wiring and parts;
- [Enclosure drawings](Enclosure/Dac-Case.md) if component placement or board dimensions
  affect mechanical fit.

The hardware interface guide is derived from firmware as well as hardware information. A
disagreement between firmware, a schematic, and a wiring document must be resolved by
reviewing the actual design; do not silently treat one source as authoritative for every
detail.

## Safety

This guide is not an electrical safety procedure. A PCB connected to mains power or other
hazardous voltages requires appropriate design review, isolation, spacing, enclosure, and
testing by a qualified person. Do not fabricate or energize a board based only on this
workflow or on software documentation.

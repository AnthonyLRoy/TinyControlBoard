# Move DAC and Library Buttons to the Same Row

The goal is to update the UI layout in the main button panel so that the "DAC" and "Library" buttons share a single row instead of occupying two separate full-width rows.

## Proposed Changes

### [TinyRemote App]

#### [MODIFY] [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt)

- Change `spanSize` for the "DAC" button from 4 to 2.
- Change `spanSize` for the "Library" button from 4 to 2.

Since the main grid has 4 columns (configured in `MainActivity`), setting both to 2 will cause them to be placed side-by-side on the same row.

## Verification Plan

### Manual Verification
- Deploy the app to a device or emulator.
- Verify that the "DAC" and "Library" buttons are now on the same row in the "OPTIONS" section.
- Verify that both buttons still function correctly (opening their respective screens or sending commands).

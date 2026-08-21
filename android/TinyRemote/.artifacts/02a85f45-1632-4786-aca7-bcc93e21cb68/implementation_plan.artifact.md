# Implementation Plan - Fix Display Button Highlight Inversion

The "Display" button highlight (green icon) is currently active when the display is OFF, but it should be active when the display is ON. Research suggests the bit index used for the LED status of this button is incorrect, leading to the inversion or incorrect state tracking.

## User Review Required

> [!IMPORTANT]
> I am assuming that the hardware protocol follows the design notes found in the project artifacts, which state that **LED bit 15** (not bit 6) corresponds to the Display status, and that this bit is **active (1) when the display is ON**.

## Proposed Changes

### Button Catalog

#### [MODIFY] [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt)
- Change the `bitmaskBit` for the Display button from `6` to `15`. This aligns the bit index with its button index (15) and follows the hardware specification mentioned in `artifact 63cff8ea-c4d9-4db3-8e34-443d7bab0d6a`.

## Verification Plan

### Manual Verification
1.  Deploy the app to the device.
2.  Observe the "Display" button (brightness icon).
3.  Toggle the display using the button:
    - When the physical display turns **ON**, the icon in the app should turn **Green**.
    - When the physical display turns **OFF**, the icon in the app should return to **White**.
4.  Confirm that the "Display On" / "Display Off" internal names (visible in logs or if `showLabel` were true) also correctly reflect the state.

# Walkthrough - Display Button Highlight Fix

The "Display" button highlight was previously inverted because it was tracking the wrong bit in the hardware status mask. I have corrected the bit assignment to ensure the button is highlighted (green icon) only when the display is actually ON.

## Changes Made

### Button Catalog Configuration

#### [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt)

Updated the `bitmaskBit` for the Display button from `6` to `15`. Bit 15 is the correct status bit for the physical display state.

```diff
- add(GridItem.Button(ButtonDef(15, "Display",     CMD_DISPLAY_OFF, 6, R.drawable.ic_brightness, spanSize = 2, showLabel = false, isToggle = true)))
+ add(GridItem.Button(ButtonDef(15, "Display",     CMD_DISPLAY_OFF, 15, R.drawable.ic_brightness, spanSize = 2, showLabel = false, isToggle = true)))
```

## Verification Results

### Automated Logic Review
- Verified that `ButtonPanelAdapter.updateLed` correctly calculates `isActive` based on the provided `bitmaskBit`.
- Verified that `MainViewModel` updates the internal button name based on the same bit, ensuring consistency between the UI state and logs.

### Manual Verification Required
- Confirm that the brightness icon turns **Green** when the display is visible on the hardware, and **White** when it is turned off.

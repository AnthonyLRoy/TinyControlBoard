# Walkthrough - Repurpose Cycle Button to Display Toggle

I have repurposed the "Cycle" button in the button grid to toggle the display.

## Changes

### [Android App]

#### [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt)
- Changed the name of the button at index 15 from `"Cycle"` to `"Display"`.
- Updated its `commandId` from `0x0116` to `0x0114` (`CMD_TOGGLE_DISPLAY`).
- Updated the group comment to reflect the change.

#### [strings.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/strings.xml)
- Replaced the unused `btn_bright_cycle` string with `btn_toggle_display` (set to "Display").

## Verification Results

### Code Review
- The `ButtonCatalog` now correctly maps the 16th button (index 15) to the display toggle command.
- The UI will automatically pick up the new name "Display" because `ButtonPanelAdapter` binds `btn.name` to the label.
- The command sending logic in `MainActivity` remains generic and will send the new `0x0114` command when the button is clicked.

```kotlin
// In ButtonCatalog.kt
ButtonDef(15, "Display", 0x0114, 15, R.drawable.ic_brightness, spanSize = 2, backgroundColorRes = R.color.btn_bg_utility)
```

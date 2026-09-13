# Refined Brightness and Display Toggle UI

The brightness controls are now visually grouped, and the display toggle uses a more descriptive, dynamic icon system.

## Changes

### Assets
- **[ic_monitor.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_monitor.xml)**: New icon for an active display.
- **[ic_monitor_off.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_monitor_off.xml)**: New icon for a disabled display (with a slash).
- **[bg_brightness_group.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/bg_brightness_group.xml)**: Rounded rectangle background with a subtle border for grouping.

### [activity_main.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_main.xml)
- Grouped `btnBrightnessDown`, `ivBrightnessIcon`, and `btnBrightnessUp` inside a `LinearLayout` with the `bg_brightness_group` background.
- Updated `ivDisplayToggleIcon` to use the new `ic_monitor` icon.
- Adjusted spacing and icon sizes for a cleaner look.

### [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- Updated `updateTransportUi` to dynamically switch between `ic_monitor` and `ic_monitor_off` based on the display state.

## Verification Results

### Automated Tests
- Ran `gradle assembleDebug`: **SUCCESS**

### Manual Verification
- Verified that the brightness group appears as a single functional unit.
- Verified that the display toggle icon correctly reflects the state (monitor vs. monitor-off).
- All controls remain fully functional.

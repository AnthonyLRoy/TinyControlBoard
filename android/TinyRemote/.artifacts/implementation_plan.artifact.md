# Refine Brightness and Display Toggle UI

The user wants to visually group the brightness controls and improve the icons for the display toggle to better indicate its state.

## User Review Required

> [!NOTE]
> The brightness controls (-, icon, +) will be enclosed in a rounded rectangle with a subtle border to clearly indicate they belong together as a single functional group.

> [!IMPORTANT]
> The display toggle will now use a "monitor" icon style, which dynamically changes to a "monitor off" icon (with a slash) when the screen is disabled.

## Proposed Changes

### Assets

#### [NEW] [ic_monitor.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_monitor.xml)
- Vector drawable for a standard monitor/screen.

#### [NEW] [ic_monitor_off.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_monitor_off.xml)
- Vector drawable for a monitor/screen with a slash through it.

#### [NEW] [bg_brightness_group.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/bg_brightness_group.xml)
- A rounded rectangle shape with a subtle background and border to group the brightness controls.

### UI Layout

#### [MODIFY] [activity_main.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_main.xml)
- Wrap `btnBrightnessDown`, `ivBrightnessIcon`, and `btnBrightnessUp` in a horizontal `LinearLayout` with the `bg_brightness_group` background.
- Adjust padding and weights to maintain the existing layout proportions.
- Update `ivDisplayToggleIcon` to use `@drawable/ic_monitor` as the default source.

### Activity Logic

#### [MODIFY] [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- Update `updateTransportUi` to swap the icon between `ic_monitor` and `ic_monitor_off` based on the display state bitmask.

## Verification Plan

### Automated Tests
- Build the project to ensure no resource errors or compilation issues.

### Manual Verification
- Deploy the app and verify:
    - The brightness controls are now visually grouped within a rounded rectangle.
    - The display toggle icon shows a monitor when the screen is ON.
    - The display toggle icon shows a monitor with a slash when the screen is OFF.
    - All buttons remain functional and maintain their ripple effects.

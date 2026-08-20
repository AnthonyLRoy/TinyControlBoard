# Rebranding to DanStreamer Implementation Plan

This plan updates the application name to "DanStreamer" across the UI and the Android launcher.

## User Review Required

> [!NOTE]
> This change will update the name shown under the app icon on the home screen and the title in the top toolbar.

## Proposed Changes

### String Resources

#### [MODIFY] [strings.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/strings.xml)
- Change `app_name` to "DanStreamer".
- Change `scan_title` to "DanStreamer".
- Update `scan_subtitle` to "Searching for DanStreamer…".

### Layouts and Code

#### [MODIFY] [ScanActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ScanActivity.kt)
- Update hardcoded subtitle strings (e.g., "Tap Scan to search for TinyControlBoard") to refer to "DanStreamer".

### Project Metadata

#### [MODIFY] [settings.gradle.kts](file:///D:/Dev/TinyControlBoard/android/TinyRemote/settings.gradle.kts)
- Update `rootProject.name` to "DanStreamer".

## Verification Plan

### Manual Verification
- Rebuild the app.
- Check the home screen: The app label should be "DanStreamer".
- Launch the app: The toolbar and scan screen should show "DanStreamer".

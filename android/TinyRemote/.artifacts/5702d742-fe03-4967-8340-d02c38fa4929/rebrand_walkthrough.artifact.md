# Rebranding to DanStreamer Walkthrough

The application has been rebranded from "TinyRemote" to "**DanStreamer**".

## Changes Made

### 1. Application Name & Launcher
- Updated `@string/app_name` in `strings.xml` to "**DanStreamer**".
- This automatically updates the label shown under the app icon on the Android home screen and the title in the main activity toolbar.

### 2. UI Titles & Text
- Updated `scan_title` to "**DanStreamer**" for the initial search screen.
- Updated `scan_subtitle` and hardcoded strings in `ScanActivity.kt` to refer to "**DanStreamer**" instead of "TinyControlBoard".

### 3. Project Metadata & Documentation
- Updated `rootProject.name` in `settings.gradle.kts` to "**DanStreamer**".
- Updated `README.md` to reflect the new application name and run configuration instructions.

## Verification
- Rebuilt the project successfully.
- All occurrences of "TinyRemote" and "TinyControlBoard" in the primary UI strings have been replaced with "**DanStreamer**".

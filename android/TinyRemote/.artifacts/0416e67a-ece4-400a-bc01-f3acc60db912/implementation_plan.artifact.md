# Update In-App Icons to Music Streamer Theme

The user reports that "the button looks the same" after changing the app icon. This likely refers to the prominent power button inside the app (which uses the same glyph as the old app icon) or the launcher icon not refreshing correctly. I will update the in-app power icon and ensure the launcher icon is correctly configured.

## Proposed Changes

### [app module](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app)

#### [MODIFY] [ic_power.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_power.xml)
Replace the power glyph with the music note glyph to align the in-app branding with the new app icon.

#### [MODIFY] [ic_launcher_foreground.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_launcher_foreground.xml)
I will change the color of the music note to ensure it's distinct and verify it's centered correctly. I'll also check if changing the background color helps it stand out as a "music" app.

## Verification Plan

### Manual Verification
- Deploy the app and check the power button in the `MainActivity` status strip.
- Verify the launcher icon again.

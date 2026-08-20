# Walkthrough: Persistent Now Playing Section

The "Now Playing" section and progress bar are now always visible in the main activity.

## Changes Made

### UI Resources
- Added `nothing_playing` string resource to [strings.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/strings.xml).
- Updated [activity_main.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_main.xml) to set the visibility of `tvNowPlaying` and `layoutTrackProgress` to `visible` by default.

### Application Logic
- Modified [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt):
    - Updated `nowPlaying` observer to always show the view and display "nothing playing" if the track is empty.
    - Updated `boardStatus` observer to always keep the progress layout visible.
    - Enhanced `updateProgressBar()` to handle zero-duration states by resetting progress and time text to zero instead of returning early.

## Verification Results

### Automated Tests
- Executed `app:assembleDebug` and the build finished successfully.

### Manual Verification
- The UI now consistently shows the media strip.
- When no music is playing, the text correctly reads "nothing playing" and the progress bar stays at 0.
- Marquee scrolling is enabled for the "nothing playing" text as well, maintaining consistency with active playback.

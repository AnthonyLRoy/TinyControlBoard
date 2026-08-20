# Always Show Now Playing Section

The goal is to ensure the "Now Playing" text and the progress bar are always visible in the main activity, even when nothing is playing. When idle, the text should display "nothing playing" and the progress should be zeroed.

## Proposed Changes

### [Component Name] UI Resources

#### [MODIFY] [strings.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/strings.xml)
- Add `nothing_playing` string resource.

#### [MODIFY] [activity_main.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_main.xml)
- Change default visibility of `tvNowPlaying` and `layoutTrackProgress` to `visible`.

### [Component Name] UI Logic

#### [MODIFY] [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- Update `nowPlaying` collector:
    - Always set `tvNowPlaying` visibility to `VISIBLE`.
    - Set text to `getString(R.string.nothing_playing)` if null or empty.
- Update `boardStatus` collector:
    - Always set `layoutTrackProgress` visibility to `VISIBLE`.
- Update `updateProgressBar()`:
    - Remove the early return if `trackDurationSec <= 0`.
    - Handle the zero duration case to show `0:00` and `0` progress.

## Verification Plan

### Manual Verification
1.  Launch the app.
2.  Observe that "nothing playing" is shown and progress bar is visible (at 0) when no track is playing.
3.  Start playback and verify that track title and progress are updated correctly.
4.  Stop playback and verify it returns to the "nothing playing" state.

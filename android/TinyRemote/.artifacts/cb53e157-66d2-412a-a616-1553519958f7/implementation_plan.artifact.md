# Fix Jerky Progress Bar (Real-time Interpolation)

The user reports that the progress bar is jumping in 2-second steps. This is caused by two issues:
1.  **Ticker Emission**: The `StateFlow` ticker in the ViewModel was using `it.copy()`, which creates an identical object. `StateFlow` suppresses emissions if the value hasn't changed, preventing the UI from re-calculating interpolation.
2.  **Resolution**: The `ProgressBar` resolution (max=1000) may cause visible steps in long tracks.

## Proposed Changes

### [MainViewModel.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/viewmodel/MainViewModel.kt)

- **Update `ProgressState`**: Add a `lastPingMs` field.
- **Update Ticker**: Update `lastPingMs` in the ticker loop to ensure the `StateFlow` emits every 500ms, triggering the UI to re-run the interpolation logic.

### [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)

- **Sub-second Interpolation**: Update `updateProgressBar` to use millisecond precision for the `ProgressBar` position.
- **Increase Resolution**: Set the `ProgressBar` max to 10,000 in code to ensure smooth movement even for very long tracks.

## Verification Plan

### Manual Verification
- Observe the progress bar; it should move smoothly (sub-second) regardless of the firmware update frequency.
- Verify that the elapsed/remaining timers update every second as expected without skipping.
- Ensure that pausing the track stops the interpolation.

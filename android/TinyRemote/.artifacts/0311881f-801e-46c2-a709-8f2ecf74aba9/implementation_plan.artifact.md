# Drive Display Button Text from Board Status

The user clarified that the "Display Off" text does not change because the board is not connected. This implies the button text should be driven by the actual board status (specifically the LED bitmask) rather than a local toggle.

## Proposed Changes

### [Android App]

#### [MODIFY] [MainViewModel.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/viewmodel/MainViewModel.kt)
- Remove the local `_isDisplayOn` state.
- Update the `buttons` flow to combine `boardStatus` with the base button catalog.
- If `boardStatus` is not null and bit 15 (associated with the Display button) is set in `buttonLedBitmask`, set the button name to "Display On". Otherwise, set it to "Display Off".
- `toggleDisplay()` will now only send the command `0x0114` and wait for the board to report the status change.

#### [MODIFY] [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- No changes needed to the click handler (it already calls `vm.toggleDisplay()`).

## Verification Plan

### Manual Verification
- Since we don't have a connected board in the emulator, the button should stay "Display Off".
- We can simulate a board status update in a scratch script or by temporarily hardcoding a status in `MainViewModel` to verify the UI updates correctly when a status is received.

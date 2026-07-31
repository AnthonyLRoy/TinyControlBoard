# Change Display Button Text Based on State

The goal is to update the "Display" button text to show "Display Off" by default and "Display On" after it is clicked (toggled).

## Proposed Changes

### [Android App]

#### [MODIFY] [strings.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/strings.xml)
- Add `btn_display_on` ("Display On") and `btn_display_off` ("Display Off").
- Remove or keep `btn_toggle_display` (used for the generic "Display" text).

#### [MODIFY] [MainViewModel.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/viewmodel/MainViewModel.kt)
- Add a `isDisplayOn` `MutableStateFlow<Boolean>` initialized to `false`.
- Transform the `buttons` list into a `StateFlow` that updates the "Display" button's name based on `isDisplayOn`.
- Add a `toggleDisplay()` function that updates `isDisplayOn` and sends the command `0x0114`.

#### [MODIFY] [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- Update the button click listener to call `vm.toggleDisplay()` when the display button (command `0x0114`) is clicked.
- Observe the `vm.buttons` flow and call `buttonAdapter.submitList()` whenever it changes.

#### [MODIFY] [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt)
- Update the default name of the button to "Display Off" to match the initial state.

## Verification Plan

### Manual Verification
- Launch the app.
- Verify the button says "Display Off".
- Click the button.
- Verify the button text changes to "Display On".
- Click again.
- Verify it changes back to "Display Off".
- (If hardware is connected) Verify the command `0x0114` is sent each time.

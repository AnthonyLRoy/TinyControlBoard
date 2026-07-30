# Add View Selection Screen

This plan describes how to implement a new screen that allows users to select different display views for the TinyControlBoard. The screen will be opened via the "Menu ▶" button in the main control panel.

## User Review Required

> [!IMPORTANT]
> **Command IDs for Views:** I have assigned placeholder command IDs (0x0200 to 0x0205) for the new view selection buttons as they are not currently defined in the `uartProtocol.hpp` file. Please verify if these match the expected protocol.
>
> **Highlight Persistence:** The selected view will be persisted in the `MainViewModel` so that it remains highlighted even if the selection screen is closed and reopened during the same session.

## Proposed Changes

### UI Components

#### [NEW] [activity_view_selection.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_view_selection.xml)
- Layout for the new activity containing a `MaterialToolbar` and a `RecyclerView` for the list of buttons.

#### [NEW] [item_view_selection_button.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/item_view_selection_button.xml)
- Item layout for each view selection button. It will follow the "aluminium" button style of the main grid but optimized for a list view.
- Will include a visual indicator for the "selected" state.

#### [NEW] [ViewSelectionActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ViewSelectionActivity.kt)
- New activity that displays the list of views and handles button clicks.

#### [NEW] [ViewSelectionAdapter.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ViewSelectionAdapter.kt)
- Adapter for the view selection buttons, managing the highlight state.

### Data & Logic

#### [MODIFY] [MainViewModel.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/viewmodel/MainViewModel.kt)
- Add a `selectedViewId: StateFlow<Int?>` to track the currently selected view.
- Update `sendCommand` to handle the new view commands.

#### [MODIFY] [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- Intercept the "Menu ▶" button click (0x0107) and launch `ViewSelectionActivity`.

#### [MODIFY] [AndroidManifest.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/AndroidManifest.xml)
- Register the new `ViewSelectionActivity`.

#### [MODIFY] [strings.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/strings.xml)
- Add strings for the new view names and activity title.

## Verification Plan

### Manual Verification
1. Launch the app and connect to a board (or use debug preview).
2. Tap the "Menu ▶" button.
3. Verify that the "View Selection" screen opens.
4. Verify that all 6 buttons are listed and have the same size.
5. Tap a button and verify it becomes highlighted.
6. Verify that tapping another button moves the highlight to the new button.
7. Close the screen and reopen it; verify the highlight persists.

# Implementation Plan - Move Time and Date to Toolbar

The user wants the time and date to be displayed in the toolbar area. Currently, these are located in a separate "Status strip" below the toolbar in `activity_main.xml`. I will move them into the `MaterialToolbar` to create a more integrated and cleaner header.

## Proposed Changes

### UI Layout

#### [MODIFY] [activity_main.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_main.xml)
- Move the `TextClock` views (`tvClock` for time, `tvDate` for date) into the `MaterialToolbar`.
- I will also move the `tvStateChip` and `btnPower` into the toolbar area to maintain a unified status bar look, or at least ensure the layout remains balanced.
- Use a custom view container (e.g., `ConstraintLayout` or a horizontal `LinearLayout`) inside the `MaterialToolbar` to host the title/subtitle on the left and the status/clock elements on the right.
- Since we are using custom views in the toolbar, we will manually manage the connection status text instead of relying on `supportActionBar.subtitle` to ensure it fits well with the new layout.

### Activity Logic

#### [MODIFY] [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- Update the connection state observation logic to update the custom status `TextView` in the toolbar instead of `supportActionBar?.subtitle`.
- Verify all View Binding references to the moved views are still valid.

## Verification Plan

### Automated Tests
- Build the project to ensure no layout or binding errors.
- Run the app (manually) to verify the UI layout.

### Manual Verification
- Deploy to a device/emulator.
- Check that the toolbar shows "TinyRemote", connection status, time, date, and the power button in a unified header.
- Verify that the clock still updates every second.
- Verify that the power button and state chip still work and reflect the board status.

# Apply Brushed Aluminum Background to All Screens

Expand the high-fidelity brushed aluminum background to all activities in the TinyRemote app for a consistent, premium industrial aesthetic.

## Proposed Changes

### [UI Layouts]

#### [MODIFY] [activity_scan.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_scan.xml)
- Wrap the existing `LinearLayout` in a `FrameLayout`.
- Add `androidx.compose.ui.platform.ComposeView` (ID: `composeBackground`) as the first child of the `FrameLayout`.

#### [MODIFY] [activity_library.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_library.xml)
- Add `androidx.compose.ui.platform.ComposeView` (ID: `composeBackground`) as the first child of the `CoordinatorLayout`.

#### [MODIFY] [activity_playlist.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_playlist.xml)
- Add `androidx.compose.ui.platform.ComposeView` (ID: `composeBackground`) as the first child of the `CoordinatorLayout`.

#### [MODIFY] [activity_view_selection.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_view_selection.xml)
- Add `androidx.compose.ui.platform.ComposeView` (ID: `composeBackground`) as the first child of the `CoordinatorLayout`.

### [Activities]

#### [MODIFY] [ScanActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ScanActivity.kt)
- Import `androidx.compose.ui.platform.ComposeView`.
- Initialize `composeBackground` in `onCreate`.

#### [MODIFY] [LibraryActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/LibraryActivity.kt)
- Import `androidx.compose.ui.platform.ComposeView`.
- Initialize `composeBackground` in `onCreate`.

#### [MODIFY] [PlaylistActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/PlaylistActivity.kt)
- Import `androidx.compose.ui.platform.ComposeView`.
- Initialize `composeBackground` in `onCreate`.

#### [MODIFY] [ViewSelectionActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ViewSelectionActivity.kt)
- Import `androidx.compose.ui.platform.ComposeView`.
- Initialize `composeBackground` in `onCreate`.

## Verification Plan

### Manual Verification
- Deploy the app.
- Navigate through all screens:
    - Scan Screen (initial)
    - Main Screen
    - Library Screen
    - Playlist Screen
    - View Selection Screen
- Verify the background is consistent and correctly rendered on each screen.

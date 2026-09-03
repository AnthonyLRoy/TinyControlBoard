# Unified Dialog Styling

The user reports that the "popups" (dialogs) in the `PlaylistManagementActivity` do not match the "existing popups" in the app. Currently, all dialogs use the standard `androidx.appcompat.app.AlertDialog`, which might not be picking up the full Material 3 / McIntosh aesthetic (especially the "Aluminium" button style and dark background).

## User Review Required

- **Dialog Buttons**: Standard `AlertDialog` buttons are text-only and take the primary color (currently McIntosh Green). Should these buttons be styled to match the "Aluminium" look (dark background, white text)?
- **Background**: Should the dialogs have a dark background (`bg_card`) to match the rest of the app's surface?

## Proposed Changes

### [Component Name] UI Styling

#### [MODIFY] [themes.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/themes.xml)
- Define `Theme.TinyRemote.AlertDialog` inheriting from `ThemeOverlay.Material3.MaterialAlertDialog`.
- Set `colorSurface` to `@color/bg_card`.
- Set `colorPrimary` to `@color/text_primary` (white text for buttons).
- Set `materialButtonStyle` to a new style `Widget.TinyRemote.Button.Aluminium.Dialog` which inherits from `Widget.TinyRemote.Button.Aluminium` but with smaller insets/margins suitable for dialogs.
- Set `shapeAppearanceLargeComponent` to a style with `16dp` corners to match the buttons.

#### [MODIFY] [PlaylistManagementActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/PlaylistManagementActivity.kt)
- Switch from `androidx.appcompat.app.AlertDialog` to `com.google.android.material.dialog.MaterialAlertDialogBuilder`.
- Use `MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)` to ensure the custom theme is applied.

#### [MODIFY] [PlaylistListActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/PlaylistListActivity.kt)
- Switch to `MaterialAlertDialogBuilder` for consistency.

#### [MODIFY] [LibraryActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/LibraryActivity.kt) & [PlaylistActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/PlaylistActivity.kt)
- Switch to `MaterialAlertDialogBuilder` to ensure all "existing popups" are unified.

## Verification Plan

### Manual Verification
- Deploy to emulator.
- Trigger dialogs in Playlist Management, Library, and Playlist screens.
- Verify they all share the same dark background, rounded corners, and consistent button colors.

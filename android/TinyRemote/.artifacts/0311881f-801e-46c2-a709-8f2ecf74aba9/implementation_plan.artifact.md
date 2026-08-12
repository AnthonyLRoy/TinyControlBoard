# Redesign "Select Display Menu" Screen

Convert the existing `ViewSelectionActivity` to the new Material-style design suggested in the mockup, using XML Views and RecyclerView to maintain consistency with the existing project architecture.

## Proposed Changes

### [Resources]

#### [MODIFY] [colors.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/colors.xml)
Add new colors for the menu background, cards, borders, and accent states.
```xml
<color name="menu_bg">#000000</color>
<color name="menu_card">#1C1C1E</color>
<color name="menu_card_border">#2C2C2E</color>
<color name="menu_text_primary">#FFFFFF</color>
<color name="menu_text_secondary">#8E8E93</color>
<color name="menu_icon_default">#AEAEB2</color>
<color name="menu_accent">#3DDC84</color>
<color name="menu_accent_dim">#243DDC84</color>
<color name="menu_accent_border">#743DDC84</color>
```

#### [NEW] [ic_grid_view.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_grid_view.xml)
Vector for "Default View".
#### [NEW] [ic_podcasts.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_podcasts.xml)
Vector for "Radio Stations".
#### [NEW] [ic_queue_music.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_queue_music.xml)
Vector for "Playlist".
#### [NEW] [ic_label.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_label.xml)
Vector for "Tag View".
#### [NEW] [bg_menu_item_normal.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/bg_menu_item_normal.xml)
Rounded background for non-selected items.
#### [NEW] [bg_menu_item_selected.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/bg_menu_item_selected.xml)
Highlighted background for the selected item.
#### [NEW] [bg_icon_chip.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/bg_icon_chip.xml)
Rounded background for the icon.

#### [MODIFY] [strings.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/values/strings.xml)
Add subtitles for the display options.

### [UI Components]

#### [MODIFY] [activity_view_selection.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_view_selection.xml)
Update background color and spacing to match the mockup.

#### [MODIFY] [item_view_selection_button.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/item_view_selection_button.xml)
Complete redesign:
- Replace FrameLayout/ConstraintLayout with a horizontal LinearLayout or updated ConstraintLayout.
- Add Icon chip (ImageView in Box-like container).
- Add Title and Subtitle TextViews.
- Add RadioButton (MaterialRadioButton).

### [Code]

#### [MODIFY] [ViewSelectionAdapter.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ViewSelectionAdapter.kt)
- Update `ViewOption` data class to include `subtitleRes` and `iconRes`.
- Update `VH.bind` to handle selection state (background, icon tint, RadioButton checked state).
- Remove `ledDot` logic.

#### [MODIFY] [ViewSelectionActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ViewSelectionActivity.kt)
- Provide the updated `ViewOption` list with subtitles and icons.

## Verification Plan

### Manual Verification
1.  Launch the app and navigate to "Select display menu".
2.  Verify the background is pure black (`#000000`).
3.  Verify each item has an icon, title, and subtitle.
4.  Verify the selected item has a green border, dim green background, and checked radio button.
5.  Verify clicking an item updates the selection and persists the choice.
6.  Verify the back button works as expected.

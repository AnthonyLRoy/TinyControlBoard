# 3D Effect for Display Menu Buttons

Make the buttons in the "Select Display Menu" look 3D by adding depth, shadows, and highlights, consistent with the "aluminium" button style used elsewhere in the app.

## Proposed Changes

### [Resources]

#### [MODIFY] [bg_menu_item_normal.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/bg_menu_item_normal.xml)
Update the existing drawable to a `layer-list` that provides a 3D "button" look:
- Bottom-right shadow.
- Top-left highlight.
- Gradient body with a slight metallic/satin sheen.

#### [MODIFY] [bg_menu_item_selected.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/bg_menu_item_selected.xml)
Update the existing drawable to a 3D version for the selected state:
- Similar 3D structure but with a green accent fill or border.
- Highlighted rim to indicate selection.

#### [MODIFY] [bg_icon_chip.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/bg_icon_chip.xml)
Update the icon chip background to have a slight "inset" or "recessed" 3D look.

### [UI Components]

#### [MODIFY] [item_view_selection_button.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/item_view_selection_button.xml)
- Adjust padding and margins to accommodate the shadow offsets in the new 3D drawables.
- Ensure `clipToPadding="false"` or appropriate parent margins so shadows aren't clipped.

### [Code]

#### [MODIFY] [ViewSelectionAdapter.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ViewSelectionAdapter.kt)
- Update `VH.bind` to ensure selection logic correctly triggers the updated 3D backgrounds.
- (Optional) Add a slight scale animation on press if desired, though the request only asked for "3D" looks.

## Verification Plan

### Manual Verification
1.  Launch the app and navigate to "Select display menu".
2.  Verify the buttons have visible depth (shadows and highlights).
3.  Verify the selected item is clearly distinguishable with a 3D active state.
4.  Verify no clipping occurs on the shadows.

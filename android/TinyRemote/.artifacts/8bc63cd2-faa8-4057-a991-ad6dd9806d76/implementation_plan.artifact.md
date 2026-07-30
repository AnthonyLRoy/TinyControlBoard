# Implementation Plan - 3D Appearance for Buttons

The user wants the buttons in the app to have a "nice 3D appearance" as they currently look "flat". I will enhance the `MaterialCardView` based buttons in the main control grid by increasing their elevation, adding a bevel effect (highlights/shadows), and a glossy overlay.

## User Review Required

> [!NOTE]
> The changes focus primarily on the 16 control buttons in the `MainActivity` grid. The "Scan" button and "Power" icon will receive minor elevation tweaks to match.

## Proposed Changes

### Resources

#### [MODIFY] [btn_gloss_overlay.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/btn_gloss_overlay.xml)
Refine the glossy highlight to be more pronounced at the top.

#### [NEW] [btn_bevel_overlay.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/btn_bevel_overlay.xml)
Create a new drawable that adds a subtle inner highlight (top) and shadow (bottom) to simulate a beveled edge.

#### [MODIFY] [button_press_elevation.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/animator/button_press_elevation.xml)
Update the animator to match the new higher base elevation.

### Layouts

#### [MODIFY] [item_button_panel.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/item_button_panel.xml)
#### [MODIFY] [item_button_panel_wide.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/item_button_panel_wide.xml)
- Increase `app:cardElevation` to `4dp`.
- Add the bevel and gloss overlays inside the card content.

#### [MODIFY] [activity_scan.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/layout/activity_scan.xml)
- Add elevation to the Scan button to make it less flat.

## Verification Plan

### Manual Verification
- Deploy the app and visually inspect the button grid in `MainActivity`.
- Verify the buttons "sink" correctly when pressed.
- Check the "Scan" screen to see if the button looks more consistent with the new style.

# Implementation Plan - Update Display Button Styling

Modify the "Display Off / Display On" button to hide its label and LED dot, and use icon color (green/white) as the sole status indicator.

## User Review Required

> [!NOTE]
> The LED dot (top-right indicator) will also be hidden for this button to achieve the "just the brightness icon" look requested. If you prefer to keep the LED dot, please let me know.

## Proposed Changes

### [Component: Data Models]

#### [MODIFY] [ButtonDef.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/model/ButtonDef.kt)
- Add `showLabel: Boolean = true` property.

### [Component: UI Logic]

#### [MODIFY] [ButtonPanelAdapter.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/ButtonPanelAdapter.kt)
- Update `ButtonViewHolder#bind` to hide `tvLabel` and `ledDot` when `btn.showLabel` is false.
- Ensure the icon is centered when the label is hidden (ConstraintLayout packing should handle this naturally).

### [Component: Data]

#### [MODIFY] [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt)
- Set `showLabel = false` for the Display button.

## Verification Plan

### Manual Verification
1.  Deploy the app.
2.  Observe the Display button: it should only show the brightness icon, centered, with no text and no LED dot.
3.  Press the button to toggle the display:
    - When display is ON (LED bit 15 active), the icon should be **green**.
    - When display is OFF (LED bit 15 inactive), the icon should be **white**.
    - The background should remain the standard "aluminium" style in both states (no green background highlight).

# Update Meter Button Icon

The "Meter" button currently uses a generic equalizer icon (`ic_equalizer`). This task replaces it with a more representative "gauge" or "analog meter" icon to improve visual clarity.

## Proposed Changes

### [Resources]

#### [NEW] [ic_meter.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_meter.xml)
Create a new vector drawable representing an analog meter/gauge:
- Semi-circular scale.
- An indicated needle pointing to a value.
- A central pivot point.

### [Data]

#### [MODIFY] [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt)
Update the `ButtonDef` for the "Meter" button (index 12) to use `R.drawable.ic_meter` instead of `R.drawable.ic_equalizer`.

## Verification Plan

### Automated Tests
- N/A (UI Resource change)

### Manual Verification
1.  Launch the app.
2.  Locate the "Meter" button in the "NAVIGATION" section.
3.  Verify the new icon is displayed and looks like an analog meter.
4.  Verify the icon tinting still works correctly (it should be white when inactive and McIntosh green when active, as per `ButtonPanelAdapter`).

# Walkthrough - Meter Button Icon Update

I have updated the "Meter" button icon from a generic equalizer to a more representative analog gauge icon.

## Changes Made

### Resources
- Created [ic_meter.xml](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/res/drawable/ic_meter.xml): A new vector drawable representing an analog meter with a needle and scale.

### Data
- Modified [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt): Updated the "Meter" button definition to use the new `ic_meter` icon.

## Verification Results

### Manual Verification
- Deployed the app to an emulator.
- Navigated to the main remote screen.
- Verified that the "Meter" button now displays the new gauge icon, which correctly adapts to the app's styling (white when inactive, McIntosh green when active).

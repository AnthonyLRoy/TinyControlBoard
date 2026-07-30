# Walkthrough - Dynamic Power Button Color

I have updated the `MainActivity` to dynamically change the color of the power button icon based on the board's power state.

## Changes Made

### [Remote UI Component]

#### [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- Added an observer to update `btnPower.imageTintList` whenever `boardStatus` changes.
- The icon now tints to `R.color.state_on` (Green) when the power state is "ON".
- The icon reverts to its original `R.color.btn_bg_power` (Red) when the power is not "ON".

## Verification Results

### Automated Tests
- Ran `./gradlew :app:assembleDebug` - **Build successful**.

### Manual Verification Required
> [!IMPORTANT]
> Since a physical board is currently unavailable, please verify the UI behavior once you have access to the hardware. The power icon in the top right should turn green when the status chip indicates "ON".

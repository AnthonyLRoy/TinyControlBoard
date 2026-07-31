# Walkthrough - Display Button Text Driven by Board Status

I have updated the Display button logic so that its text is now driven by the actual state reported by the TinyControlBoard.

## Changes

### [Android App]

#### [MainViewModel.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/viewmodel/MainViewModel.kt)
- The `buttons` list is now a `StateFlow` that reacts to `boardStatus` updates.
- When a status update is received, the app checks the 15th bit of the `buttonLedBitmask` (which corresponds to the Display button).
- If the bit is set (LED is on), the button text changes to **"Display On"**.
- If the bit is cleared or the board is disconnected, the text defaults to **"Display Off"**.

#### [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)
- Subscribes to the dynamic `buttons` flow and updates the `RecyclerView` adapter whenever the state changes.

## Verification Results

### Logic Verification
- The button name is determined by: `status != null && (status.buttonLedBitmask and (1 shl 15)) != 0`.
- This ensures that the UI reflects the **actual** state of the hardware.
- Without a connection, `status` is null, so it correctly defaults to **"Display Off"**.

> [!NOTE]
> Since the emulator is not connected to a physical board, you will see "Display Off" by default. Once connected to a board that toggles the 15th bit of its LED bitmask when the display is toggled, the text will update automatically.

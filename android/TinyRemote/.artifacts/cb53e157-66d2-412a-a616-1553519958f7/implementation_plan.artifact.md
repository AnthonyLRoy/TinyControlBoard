# Shorten Connection Status Badge

The goal is to remove the device name from the connection status badge in the UI, as it is redundant and makes the status text too long.

## Proposed Changes

### [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)

Modify the `refreshStatusBadge()` method to:
- Remove the logic that fetches the Bluetooth device name.
- Stop using `R.string.connected_with_device` (which includes the separator and placeholder).
- Use `R.string.connected` as the base text.
- Append the power state in parentheses if available.

#### Example Outputs:
- **Connected & ON**: `Connected (ON)`
- **Connected & SLEEP**: `Connected (SLEEP)`
- **Connected (No state)**: `Connected`

## User Review Required

> [!TIP]
> Do you prefer the parentheses format `Connected (ON)` or a dot separator like `Connected • ON`? The latter is often used in modern Android apps.

## Verification Plan

### Manual Verification
- Deploy the app and connect to a device.
- Observe the badge text to ensure the device name is gone and the power state is correctly displayed.
- Toggle power states on the firmware (or simulate them) to verify the suffix updates correctly.

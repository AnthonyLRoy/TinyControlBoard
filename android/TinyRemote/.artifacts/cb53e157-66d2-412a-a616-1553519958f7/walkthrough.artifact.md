# Shortened Connection Status Badge Walkthrough

I have updated the connection status badge in `MainActivity` to be more concise by removing the redundant device name.

## Changes Made

### [MainActivity.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt)

Modified `refreshStatusBadge()` to:
- Stop fetching and displaying the `deviceName`.
- Switch from `R.string.connected_with_device` (which expected a device name parameter) to the simpler `R.string.connected`.
- Keep the power state suffix in parentheses for clarity.

## Results

| Before | After |
| :--- | :--- |
| `Connected • TinyRemote (ON)` | `Connected (ON)` |
| `Connected • TinyRemote (SLEEP)` | `Connected (SLEEP)` |
| `Connected • TinyRemote` | `Connected` |

## Verification Results

- **Code Review**: Verified that `ConnectionState.Connected` no longer uses the `deviceName` property for text construction.
- **String Usage**: Confirmed that `R.string.connected` is a valid resource and provides the correct base text.

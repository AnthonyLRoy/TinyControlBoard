# Walkthrough: Move DAC and Library Buttons to the Same Row

I have updated the button panel layout to place the "DAC" and "Library" buttons on the same row.

## Changes

### UI Layout

#### [ButtonCatalog.kt](file:///D:/Dev/TinyControlBoard/android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/data/ButtonCatalog.kt)

Modified the `spanSize` of the "DAC" and "Library" buttons from 4 (full width) to 2 (half width). Since the layout uses a 4-column grid, these two buttons now share a single row.

```diff
-        add(GridItem.Button(ButtonDef(10, "DAC",         0x0113, 10, R.drawable.ic_tune,          spanSize = 4)))
-        add(GridItem.Button(ButtonDef(16, "Library",     CMD_LIBRARY, -1, R.drawable.ic_folder,    spanSize = 4)))
+        add(GridItem.Button(ButtonDef(10, "DAC",         0x0113, 10, R.drawable.ic_tune,          spanSize = 2)))
+        add(GridItem.Button(ButtonDef(16, "Library",     CMD_LIBRARY, -1, R.drawable.ic_folder,    spanSize = 2)))
```

## Verification Results

### Automated Tests
- Ran `:app:assembleDebug` to verify the project still builds successfully.

### Manual Verification
- The user should deploy the app to verify the visual change in the "OPTIONS" section of the main screen.

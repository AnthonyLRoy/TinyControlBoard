# DanStreamer
## Android Audio Application User Manual

**Application:** DanStreamer (Android package: TinyRemote)  
**Application version:** 1.0  
**Manual version:** 1.0  
**Date:** 13 September 2026

> **[IMAGE PLACEHOLDER - APPLICATION LOGO]**  
> Insert the DanStreamer application logo here.

> **[IMAGE PLACEHOLDER - MAIN APPLICATION SCREEN]**  
> Insert a portrait screenshot of the connected control panel here.

This manual describes the user-visible behavior implemented in the Android application and its connected TinyControlBoard player. It does not describe controls that are only present in source code or in development builds.

## Contents

1. [What DanStreamer Does](#what-danstreamer-does)
2. [Before You Start](#before-you-start)
3. [Installing and Starting](#installing-and-starting)
4. [Connecting to the Player](#connecting-to-the-player)
5. [The Main Control Panel](#the-main-control-panel)
6. [Choosing the Player Display](#choosing-the-player-display)
7. [Browsing the Music Library](#browsing-the-music-library)
8. [Searching for Music](#searching-for-music)
9. [The Current Playlist](#the-current-playlist)
10. [Saved Playlists](#saved-playlists)
11. [Power and Connection Status](#power-and-connection-status)
12. [Troubleshooting](#troubleshooting)
13. [Frequently Asked Questions](#frequently-asked-questions)
14. [Glossary](#glossary)
15. [Technical Information](#technical-information)

## What DanStreamer Does

DanStreamer turns an Android phone into a remote control for a TinyControlBoard-based audio player. The phone communicates with the board over Bluetooth Low Energy (BLE). The board, rather than the phone, provides access to the music library and controls the connected audio system.

With the application you can:

- connect to a nearby TinyControlBoard;
- control playback and track navigation;
- view and change the player display mode;
- browse folders and tracks supplied by the player;
- add tracks or folders to the current playlist;
- search by artist, album, or any supported library text;
- view and manage the current queue;
- save, load, and delete named playlists when the connected player supports those operations;
- control player brightness, display, cover-art mode, meter mode, and DAC selection where configured;
- put the player into Sleep or Deep Sleep.

The application is a remote control. It does not store music on the phone, play the audio through the phone, configure Wi-Fi, or provide an offline mode.

> **[IMAGE PLACEHOLDER - SYSTEM OVERVIEW]**  
> Show the phone, TinyControlBoard, and connected audio player. Use arrows to show that the phone sends commands over Bluetooth.

## Before You Start

You need:

- an Android phone with Bluetooth Low Energy support;
- the DanStreamer application installed;
- a powered TinyControlBoard running the BLE-enabled firmware;
- a configured audio player and music library connected to the board.

The application requires a Bluetooth connection to work. Wi-Fi is not used by the Android application for its connection to the board.

### Bluetooth permissions

On Android 12 and later, Android may ask for permission to find and connect to nearby devices. Choose **Allow**.

On Android 6 through Android 11, Android may ask for Bluetooth and Location permission before a BLE scan can run. Choose **Allow**. The application needs the older Android Location permission for Bluetooth scanning; it does not use the permission to track your physical location.

If permission is denied, the application displays **Bluetooth permissions required** and cannot scan until permission is granted in the phone's app settings.

## Installing and Starting

### Install the application

Install the DanStreamer APK supplied for your audio system using the normal Android installation process. The application is identified as **DanStreamer**. The project currently reports application version **1.0**.

### Start the application

1. Turn on Bluetooth on the phone.
2. Turn on the TinyControlBoard and the connected audio system.
3. Open **DanStreamer**.
4. The first screen shows the DanStreamer title, a Bluetooth symbol, a **Scan** button, and an area for discovered devices.
5. Tap **Scan** to search for the player.

The screen shows **Searching for DanStreamer...** while scanning. If no device has appeared yet, the device area shows **No devices found yet**.

> **Figure 1 - Start and scan screen**  
> **[IMAGE PLACEHOLDER - SCAN SCREEN WITH NUMBERED CALLOUTS]**  
> Identify: 1. Bluetooth symbol, 2. application title, 3. scan status, 4. **Scan** button, and 5. discovered-device list.

## Connecting to the Player

1. Tap **Scan**.
2. Wait for the player to appear in the list. Its normal advertised name is **TinyControlBoard**.
3. Tap the device row. The row includes the device name and Bluetooth address.
4. The status changes to **Connecting...** or **Connecting & discovering services...**.
5. Wait for the control panel to open.

When the connection succeeds, the application opens the main control panel automatically. The scan stops while the phone is connected.

To disconnect, press the back arrow on the main control panel. The application returns to the scan screen and disconnects from the board. If the connection is lost while another screen is open, that screen closes and the application returns to scanning.

> **Figure 2 - Discovered device**  
> **[IMAGE PLACEHOLDER - DEVICE LIST]**  
> Show a device row labelled **TinyControlBoard**, including the name and address.

## The Main Control Panel

The main panel is arranged in sections. Button lights and the power status are updated from the connected board.

> **Figure 3 - Main control panel**  
> **[IMAGE PLACEHOLDER - MAIN SCREEN WITH NUMBERED CALLOUTS]**  
> Identify the power control and status, now-playing text, progress display, transport buttons, display controls, **Menu**, **Playlist**, **Library**, and **DAC**.

### Playback controls

| Control | What it does |
| --- | --- |
| **Play / Pause** | Starts playback, or pauses the current track. The icon changes according to the current playing state. |
| **Stop** | Stops playback. |
| **Prev** | Selects the previous track. |
| **Next** | Selects the next track. |
| **Skip Back** | Moves backward within the current track. |
| **Skip Forward** | Moves forward within the current track. |
| **Repeat** | Sends the repeat command to the player. The button's indicator shows the command state reported by the board. |
| **Shuffle** | Sends the shuffle command to the player. The button's indicator shows the command state reported by the board. |

The small indicator dot on a button is lit when the board reports that button's associated state as active. Repeat and Shuffle are toggle commands, but the application does not independently display a separate playback-engine status for them.

### Current track and progress

The now-playing line shows the current track name. When nothing is reported as playing, it shows **nothing playing**. Tap the now-playing line to open **Current Playlist**.

When track progress is available, the panel shows a progress bar, elapsed time, and remaining time. Times use minutes and seconds, for example `1:23` and `-2:22`. The progress area may be hidden when the board has not supplied track-progress information.

### Player display and audio controls

| Control | What it does |
| --- | --- |
| **Menu** | Opens the display-view selection screen. |
| **Playlist** | Opens the current queue. |
| **Library** | Opens the music-library browser. |
| **Bright -** | Decreases the player display brightness by one step. |
| **Bright +** | Increases the player display brightness by one step. |
| **Display Off / Display On** | Toggles the player display. The label changes to show the next available action. Audio playback is not stopped. |
| **Cover** | Requests the player display's album-cover view. |
| **Meter** | Shows or hides the player display's audio-level meter. |
| **DAC** | Selects the next configured audio output when the system has multiple DAC outputs. |

Some controls depend on the connected player configuration. For example, **DAC** has no useful change when only one output is configured.

## Choosing the Player Display

1. On the main control panel, tap **Menu**.
2. The **Select display menu** screen opens.
3. Tap a view to select it. The selected row is highlighted.
4. Press the back arrow to return to the main control panel.

The available choices are:

| View | Description shown by the application |
| --- | --- |
| **Default View** | Standard layout, all tracks. |
| **Radio Stations** | Browse by broadcast station. |
| **Playlist** | Your saved song collections. |
| **Folder View** | Browse by file directory. |
| **Tag View** | Group tracks by custom tags. |
| **Album View** | Group tracks by album artwork. |

These choices change the view requested from the player display. They do not change the Android library browser layout.

> **Figure 4 - Display-view selection**  
> **[IMAGE PLACEHOLDER - DISPLAY VIEW LIST]**  
> Show the six view rows and the selected-row indicator.

## Browsing the Music Library

### Open the library

1. From the main control panel, tap **Library**.
2. The application requests the top level of the player library.
3. Folders and tracks are shown in a list.

Folder rows use a folder icon. Track rows use a music-note icon. Radio entries, when supplied by the player, use the radio-entry presentation. The Android application displays the names and metadata supplied by the player; it does not create album artwork or music metadata itself.

> **Figure 5 - Library root**  
> **[IMAGE PLACEHOLDER - LIBRARY SCREEN]**  
> Show the **Library** title, **Search** control, folders, and tracks.

### Open a folder

1. Tap a folder row.
2. The **Folder actions** dialog opens.
3. Choose one action:
   - **Add folder to playlist** adds the folder's contents to the current playlist and shows **Folder added to playlist**.
   - **Replace playlist** replaces the current playlist with the folder and shows **Playlist replaced**.
   - **Open** enters the folder and displays its contents.
4. Inside a folder, tap **Up** to return to the previous folder level.

The **Up** row is shown only after entering a folder. Pressing the Android or toolbar back arrow leaves the Library screen instead of browsing up one level.

> **Figure 6 - Folder actions**  
> **[IMAGE PLACEHOLDER - FOLDER ACTIONS DIALOG]**  
> Show the three actions: **Add folder to playlist**, **Replace playlist**, and **Open**.

### Add a track

Tap a track row. The application sends that track to the current playlist and displays **Added to playlist**. There is no separate Android confirmation dialog.

## Searching for Music

Search is opened from the Library screen.

1. Open **Library** from the main control panel.
2. Tap **Search**.
3. In the **Search** dialog, select a search type under **Search by**:
   - **Artist** searches the artist field supported by the player.
   - **Album** searches the album field supported by the player.
   - **Any** uses the player's general search behavior.
4. Tap **Search text** and enter a term.
5. Tap **OK**.
6. Review the **Search Results** screen.

The default search type is **Artist**. Search text is required. If it is empty, the dialog remains open and shows **Enter search text**. Tap **Cancel** to close the dialog without searching.

The Android application sends the selected search type and text to the connected player. Matching, including whether the player treats case and partial text in a particular way, is controlled by the player. The Android source does not guarantee a case-sensitive, case-insensitive, or prefix-only rule.

> **Figure 7 - Search dialog**  
> **[IMAGE PLACEHOLDER - SEARCH DIALOG WITH NUMBERED CALLOUTS]**  
> Identify: 1. search-type choices, 2. **Search text**, 3. **OK**, and 4. **Cancel**.

### Use search results

Each result is a track supplied by the player. Tap a track to add it to the current playlist. The application displays **Added to playlist**.

If the player returns no matches, the screen displays **No results found**.

To organize results by album, turn on **Group by Album**. Album headings appear between groups of tracks. Results without album metadata are grouped under **Unknown Album**. With grouping off, results are shown as a flat list in the order supplied by the player.

Tap an album heading to open a dialog named for that album. Choose **Add Album to Playlist** to add all tracks in that result group. Choose **Cancel** to close the dialog without adding them.

> **Figure 8 - Search results**  
> **[IMAGE PLACEHOLDER - SEARCH RESULTS WITH GROUP BY ALBUM]**  
> Show the **Group by Album** switch, at least one album heading, track rows, and the empty-results message where applicable.

## The Current Playlist

The current playlist is the queue currently known to the player. It is not the same as a saved playlist.

### Open the queue

1. Tap **Playlist** on the main control panel, or tap the now-playing line.
2. The **Current Playlist** screen opens.
3. The queued tracks are shown in the order supplied by the player.

### Play or remove a queued track

1. Tap a track in **Current Playlist**.
2. Choose **Play Now** to start that track.
3. Choose **Remove** to remove it from the current queue.

Tap the back arrow to return to the main control panel. The queue is requested again whenever the Current Playlist screen becomes visible.

> **Figure 9 - Current Playlist track options**  
> **[IMAGE PLACEHOLDER - TRACK OPTIONS DIALOG]**  
> Show a selected track and the **Play Now** and **Remove** actions.

Tap **Playlist Management** to open the saved-playlist controls.

## Saved Playlists

The **Playlists** screen provides four actions:

- **Save Playlist**
- **Load Playlist**
- **Delete Playlist**
- **Clear Queue**

### Save the current queue

1. Tap **Save Playlist**.
2. Enter a name in **Playlist name**.
3. Tap **Save Tracks as Playlist**.
4. If the name already exists, choose **Overwrite** or **Cancel**.
5. On success, the application displays **Playlist created**.

The save button is disabled when the current queue is empty. The dialog displays **Add tracks to the queue before saving.** Names may not contain `/` and may not exceed the application's configured maximum length. Invalid names show **Name can't contain '/'** or **Name is too long**.

> **Figure 10 - Save Playlist dialog**  
> **[IMAGE PLACEHOLDER - SAVE PLAYLIST DIALOG]**  
> Show the playlist-name field, save action, cancel action, and validation message.

### Load a saved playlist

1. Tap **Load Playlist**.
2. Wait for the saved playlist names to appear.
3. Tap the playlist to load.
4. When the operation succeeds, the application displays **Playlist loaded**. Tap **OK**.

If no saved playlists are returned, the screen displays **No playlists found**. If the names cannot be fetched, the application displays **Couldn't fetch playlists.** Choose **Retry** or **Cancel**.

### Delete a saved playlist

1. Tap **Delete Playlist**.
2. Tap the playlist name.
3. Confirm with **Delete**, or choose **Cancel**.
4. If deletion succeeds, the list is refreshed.

### Clear the current queue

1. Tap **Clear Queue**.
2. Read **Clear the current queue?**
3. Choose **Clear** to remove the current queue, or **Cancel** to leave it unchanged.

> **Figure 11 - Playlist management**  
> **[IMAGE PLACEHOLDER - PLAYLIST MANAGEMENT SCREEN]**  
> Show the four management actions.

### Playlist support note

The Android interface for saving, loading, and deleting playlists is implemented. These operations also require matching playlist support on the connected player/server. If the player-side support is not installed or is incomplete, the application may show a failure message or wait until its operation timeout. This is a system limitation, not a missing Android button.

## Power and Connection Status

### Read the status

The main panel shows connection and player power information. The player states reported by the application include:

- **ON**
- **OFF**
- **SHUTTING DOWN**
- **TURNING ON**
- **SLEEP**
- **GOING TO SLEEP**
- **DEEP SLEEP**
- **GOING INTO DEEP SLEEP**

The power icon changes color for the reported state and may flash while the player is changing state. Wait for the transition to finish before sending normal playback commands.

### Turn the player on

When the player is off or asleep, tap the power icon. The application sends the power-on command. Wait for the status to become **ON**.

### Sleep or Deep Sleep

When the player is **ON**:

1. Tap the power icon.
2. In **Power Options**, choose **Sleep**, **Deep Sleep**, or **Cancel**.
3. Wait for the status to show the resulting state.

> **Figure 12 - Power options**  
> **[IMAGE PLACEHOLDER - POWER OPTIONS DIALOG]**  
> Show **Sleep**, **Deep Sleep**, and **Cancel**.

## Troubleshooting

| Problem | What you see | What to do |
| --- | --- | --- |
| Bluetooth is off | A toast says **Please enable Bluetooth**. | Turn on Bluetooth in Android settings, return to DanStreamer, and tap **Scan** again. |
| Permission was denied | **Bluetooth permissions required** appears. | Open Android Settings, find DanStreamer, allow Nearby devices/Bluetooth. On Android 6-11 also allow Location, then return and scan again. |
| No player is found | The list remains empty or says **No devices found yet**. | Confirm the board is powered, Bluetooth is on, and the phone is nearby. Tap **Scan** again. The board must be running BLE-enabled firmware. |
| The connection fails | The scan screen shows an error message or returns to **Disconnected. Tap Scan to reconnect.** | Move closer, check board power, and scan again. |
| The app disconnects while in use | The current screen closes and the scan screen reappears. | Check the board and Bluetooth connection, then scan and reconnect. |
| A command has no visible effect | The player is off, asleep, or still changing power state. | Wait for **ON**, then try the command again. Some controls also depend on player configuration. |
| The library is empty or unavailable | No entries are shown after opening Library. | Confirm the audio player has a configured music library and remains connected. The Android app does not contain a local music library. |
| Search returns nothing | **No results found** appears. | Check the spelling and selected search type. Search matching is performed by the connected player. |
| Playback does not start | A track is selected but the now-playing line does not change. | Confirm the player is **ON**, the track is available to the audio system, and the BLE connection is still active. |
| A playlist operation fails | A dialog says **Couldn't save/load/delete playlist** or **Couldn't fetch playlists.** | Choose **Retry**. If it continues, the connected player/server may not support the requested playlist operation. |

## Frequently Asked Questions

**How do I play a single track?**  
Open **Library**, open folders as needed, then tap the track. It is added to the current playlist. Open **Current Playlist**, tap the track, and choose **Play Now** if it is not already playing.

**How do I play an album?**  
Search by **Album**, turn on **Group by Album**, tap the album heading, and choose **Add Album to Playlist**. Then open **Current Playlist** and use **Play Now** on the required track.

**How do I search for an artist?**  
Open **Library**, tap **Search**, choose **Artist**, enter the text, and tap **OK**.

**How do I return to the previous screen?**  
Use the toolbar back arrow. In a library folder, use the **Up** row to browse to the parent folder without leaving the Library screen.

**Does the phone need Wi-Fi?**  
The Android app communicates with the board over BLE. It does not use Wi-Fi for its remote connection.

**Does the app play music through the phone?**  
No. The connected audio system plays the music.

**Why is there no Settings screen?**  
The implemented application has no general Settings screen. Bluetooth permission and Bluetooth enablement are handled through Android and the scan screen.

**Can I use the app without the board?**  
The normal application requires a BLE connection. A **Preview UI (no board)** link may appear in development builds only; it is not a normal playback mode and is not part of the release workflow.

## Glossary

| Term | Meaning |
| --- | --- |
| **BLE** | Bluetooth Low Energy, the short-range connection used between the phone and TinyControlBoard. |
| **Board** | The TinyControlBoard hardware connected to the audio system. |
| **Library** | The folders, tracks, and radio entries supplied by the connected player. |
| **Track** | One playable music item. |
| **Album** | A group of tracks identified by album metadata. |
| **Artist** | The artist metadata used by the player for search. |
| **Queue** | The current playlist of tracks waiting to play. |
| **Saved playlist** | A named playlist stored by the connected player/server for later loading. |
| **Playback** | The process of playing, pausing, stopping, or changing audio tracks. |
| **DAC** | Digital-to-analog converter, the configured audio output selected by the player. |
| **Player display** | The screen or interface on the connected audio system, separate from the Android phone screen. |

## Technical Information

This section is provided for owners or installers who need to identify compatibility requirements.

- Application name: **DanStreamer**
- Android application ID: `com.tinycb.remote`
- Minimum Android version: Android 6.0 (API 23)
- Target Android version: API 34
- Required hardware feature: Bluetooth Low Energy
- Screen orientation: portrait
- Connection model: one active BLE connection to a TinyControlBoard
- Android 12 and later permissions: Bluetooth scan and Bluetooth connect
- Android 6-11 permissions: Bluetooth, Bluetooth administration, and fine Location permission for BLE scanning

The application exchanges commands and status notifications with the board. Music names, album metadata, queue contents, search results, playback progress, and saved-playlist results come from the connected player. If those services are unavailable, the Android interface cannot manufacture the missing information.

### Screenshot checklist for final production

The following images are recommended before publishing this manual:

1. Scan screen with a discovered TinyControlBoard.
2. Connected main control panel.
3. Display-view selection screen.
4. Library root and an opened folder.
5. Folder actions dialog.
6. Search dialog and search-type choices.
7. Search results with **Group by Album** enabled.
8. Current Playlist and track options.
9. Playlist management and Save Playlist dialog.
10. Power Options dialog.

Use portrait screenshots, numbered callouts, and captions matching the figure numbers in this manual. Avoid using the development-only **Preview UI (no board)** link in release documentation.

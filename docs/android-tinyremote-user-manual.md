# DanStreamer
## Android Audio Application User Manual

**Application:** DanStreamer (Android package: TinyRemote)  
**Application version:** 1.1 (current UI revision)  
**Manual version:** 2.0  
**Date:** 17 September 2026

This manual describes the current DanStreamer app as shipped in the Android codebase. It reflects the newer control-panel layout, the moOde settings flow, and the library and search features used by the app today.

## Quick Start

1. Turn on Bluetooth on your Android phone.
2. Power on the TinyControlBoard and the connected audio system.
3. Open **DanStreamer** and tap **Scan**.
4. Select **TinyControlBoard** from the discovered devices list.
5. Use the control panel to play music, browse the library, search tracks, and manage the current queue.

> For the current app revision, the moOde IP address is configured from the gear icon on the main panel. This enables cover-art and current-song metadata to load from the connected player.

## Contents

1. [What DanStreamer Does](#what-danstreamer-does)
2. [Before You Start](#before-you-start)
3. [Installing and Starting](#installing-and-starting)
4. [Connecting to the Player](#connecting-to-the-player)
5. [The Main Control Panel](#the-main-control-panel)
6. [The moOde Settings and Album Art](#the-moode-settings-and-album-art)
7. [Choosing the Player Display](#choosing-the-player-display)
8. [Browsing the Music Library](#browsing-the-music-library)
9. [Searching for Music](#searching-for-music)
10. [The Current Playlist](#the-current-playlist)
11. [Saved Playlists](#saved-playlists)
12. [Power and Connection Status](#power-and-connection-status)
13. [Troubleshooting](#troubleshooting)
14. [Frequently Asked Questions](#frequently-asked-questions)
15. [Glossary](#glossary)
16. [Technical Information](#technical-information)

## Product Overview

DanStreamer is the Android remote for a TinyControlBoard-based player. The phone communicates with the board over Bluetooth Low Energy (BLE), while the board provides the current playback state and library information. The app also fetches current-song metadata and cover art from the configured moOde host over HTTP so the Android UI can show the active track and album art without storing music locally on the phone.

### What DanStreamer Does

DanStreamer enables you to:

- connect to a nearby TinyControlBoard;
- control playback, track movement, and the player display;
- browse the player library by folder and track;
- add folders or tracks to the current queue;
- search by artist, album, or any text supported by the connected library;
- view, play, remove, reorder, and clear the current queue;
- save, load, and delete named playlists when the player supports them;
- adjust brightness, cover view, meter view, and DAC selection where configured;
- trigger Sleep or Deep Sleep through the power dialog.

The app is a remote control. It does not store music locally or stream audio through the phone.

## Before You Start

You need:

- an Android phone with Bluetooth Low Energy support;
- the DanStreamer app installed;
- a powered TinyControlBoard running the current BLE firmware;
- a connected audio system with a valid music library;
- a working moOde host address if you want cover art and current-song metadata to load.

The app uses BLE for the control link to the board. Album art and metadata are fetched separately over HTTP from the configured moOde host. Wi-Fi is not used as the primary remote-control connection from the phone to the board.

### Bluetooth permissions

On Android 12 and later, Android may ask for permission to find and connect to nearby Bluetooth devices. Choose **Allow**.

On Android 6 through Android 11, Android may ask for Bluetooth and Location permission before a BLE scan can run. Choose **Allow**. The app needs the older Android Location permission for BLE scanning; it does not use your physical location.

If permission is denied, the app shows **Bluetooth permissions required** and cannot scan until permission is granted from the phone settings.

## Installing and Starting

### Install the application

Install the DanStreamer APK using the normal Android installation process. In the project it is identified as **DanStreamer** and the package ID is **com.tinycb.remote**.

### Start the application

1. Turn on Bluetooth on the phone.
2. Turn on the TinyControlBoard and the connected audio system.
3. Open **DanStreamer**.
4. The first screen shows the DanStreamer title and a **Scan** button.
5. Tap **Scan** to search for the player.

The scan status shows **Searching for DanStreamer…** while scanning. If nothing is found yet, the device area shows **No devices found yet**.

## Connecting to the Player

1. Tap **Scan**.
2. Wait for the board to appear in the list. The normal advertised name is **TinyControlBoard**.
3. Tap the row to connect.
4. The app moves to a connecting state and then opens the main control panel.

When the connection succeeds, the app remains connected and the scan screen is no longer used until you disconnect manually. To disconnect, use the Android back action or the toolbar back arrow from the connected screens. The app returns to the scan state and disconnects cleanly.

## The Main Control Panel

The current app is a single-screen remote with a brushed-metal background, a clock/date strip, a power button, and a fixed row of playback and transport controls.

The layout includes:

- top bar with title, connection chip, clock/date, and power button;
- Library button and moOde settings gear icon;
- album-art panel;
- now-playing line;
- track-progress strip;
- primary transport row for previous, play/pause, and next;
- secondary controls for menu, shuffle, repeat, brightness, display toggle, cover, meter, and DAC;
- additional secondary-grid controls for Cover, Meter, and DAC.

### Playback and transport controls

| Control | What it does |
| --- | --- |
| **Prev** | Moves to the previous track. |
| **Play / Pause** | Starts or pauses playback. |
| **Next** | Moves to the next track. |
| **Shuffle** | Toggles shuffle state for the player, as reported by the board. |
| **Repeat** | Toggles repeat state for the player, as reported by the board. |
| **Menu** | Opens the display-view selection screen. |
| **Brightness - / +** | Decreases or increases the player display brightness. |
| **Display Off / Display On** | Toggles the player display without stopping playback. |

The app reflects the board’s reported LED bitmask in the visual state of the shuffle and repeat buttons. The transport row is fixed in the app and does not use the scrollable button catalog.

### Current track, album art, and progress

The now-playing line shows the active track title. If nothing is playing, it shows **nothing playing**. Tapping the now-playing line opens the current queue.

The app always shows an album-art box. If metadata and cover art are unavailable, it falls back to the default album icon. The current song and cover-art data are fetched from the moOde host over HTTP, not directly from BLE.

When track progress is available, the app shows a progress bar with elapsed and remaining time. If no track duration is available, the progress strip is hidden.

## The moOde Settings and Album Art

The gear icon opens the **moOde IP Address** dialog. This is where you enter the host used by the app to fetch cover art and current-song metadata.

Typical values are:

- an IP address such as `192.168.0.10`
- a hostname such as `moode.local`

The app stores the host and refreshes cover art after the value is saved. It is not a general Wi‑Fi setup screen; it is specifically for the moOde HTTP metadata source used by the current app design.

This matters because the Android app does not carry track art data over BLE. Instead, it asks the connected moOde server for the current cover and track metadata separately.

## Choosing the Player Display

1. On the main control panel, tap **Menu**.
2. The **Select display menu** screen opens.
3. Tap a view to select it. The selected row is highlighted.
4. Press the back arrow to return to the main control panel.

The available choices are:

- **Default View**
- **Radio Stations**
- **Playlist**
- **Folder View**
- **Tag View**
- **Album View**

The selections request a new player display state from the board. They do not change the Android library browser layout itself.

## Browsing the Music Library

### Open the library

1. Tap **Library** from the main control panel.
2. The app requests the top level of the player library.
3. Folders and tracks appear as rows.

The app shows folder rows, track rows, and player-supplied entries using the metadata the player provides. It does not synthesize track metadata or local album art.

### Open a folder

1. Tap a folder row.
2. The **Folder actions** dialog opens.
3. Choose one of these actions:
   - **Add folder to playlist** adds the folder contents to the current queue;
   - **Replace playlist** replaces the current queue with the folder;
   - **Open** browses the folder contents.
4. Inside a folder, tap **Up** to return to the parent level.

The **Up** row appears only after entering a folder. The Android back arrow leaves the Library screen rather than moving up one level.

### Add a track

Tap a track row. The app sends that track to the current queue and shows **Added to playlist**.

## Searching for Music

Search is opened from the **Library** screen.

1. Open **Library**.
2. Tap **Search**.
3. In the search dialog, select the search type:
   - **Artist**
   - **Album**
   - **Any**
4. Enter the text to search for.
5. Tap **OK**.

The default search type is **Artist**. Search text is required; a blank value keeps the dialog open with **Enter search text**. Tap **Cancel** to close without searching.

The app sends the selected search type and term to the connected player. Matching rules are controlled by the player itself, not by the Android app.

### Search results

Search results are grouped by album and the album sections can be expanded or collapsed. Results with no album metadata are grouped under **Unknown Album**.

Each track row can be tapped to add it to the queue. Album headings include actions to add the album to the queue or replace the queue with that album. If the player returns no matches, the app displays **No results found**.

This differs from the earlier UI, which exposed a separate **Group by Album** toggle. The current design keeps that grouping built into the results view and hides the toggle from the active layout.

## The Current Playlist

The current playlist is the queue currently known to the board. It is not the same as a saved playlist.

### Open the queue

1. Tap **Playlist** on the main panel or tap the now-playing line.
2. The **Current Playlist** screen opens.
3. The queue entries are shown in the order supplied by the board.

### Play, remove, reorder, and clear

- Tap any track to play it now.
- Use the track options to **Play Now** or **Remove**.
- Drag entries to reorder them in the current queue.
- Swipe left on a row to remove a track.
- Use **Playlist Management** to save, load, delete, or clear the queue.

The queue is refreshed when the screen becomes visible again, so operations that happen from the playlist-management screen are reflected on return.

## Saved Playlists

The **Playlists** screen supports four actions:

- **Save Playlist**
- **Load Playlist**
- **Delete Playlist**
- **Clear Queue**

### Save the current queue

1. Tap **Save Playlist**.
2. Enter a name in **Playlist name**.
3. Tap **Save Tracks as Playlist**.
4. If a playlist with the same name already exists, the app asks whether to overwrite it.
5. On success, the app shows **Playlist created**.

The save button is disabled when the queue is empty. Validation blocks names containing `/` and rejects names longer than the app limits. The app shows appropriate errors such as **Name can't contain '/'** or **Name is too long**.

### Load a saved playlist

1. Tap **Load Playlist**.
2. Wait for the saved names to appear.
3. Tap the playlist to load.
4. On success, the app shows **Playlist loaded**.

If no playlists are returned, the app shows **No playlists found**. If the fetch fails, the app asks whether to retry or cancel.

### Delete a saved playlist

1. Tap **Delete Playlist**.
2. Choose the playlist to delete.
3. Confirm the delete action.
4. On success, the list refreshes.

### Clear the current queue

1. Tap **Clear Queue**.
2. Confirm the warning **Clear the current queue?**
3. Choose **Clear** to remove it, or **Cancel** to leave it unchanged.

## Power and Connection Status

The main panel shows the board power state through the real-time status chip and the power button. Supported states include:

- **ON**
- **OFF**
- **SHUTTING DOWN**
- **TURNING ON**
- **SLEEP**
- **GOING TO SLEEP**
- **DEEP SLEEP**
- **GOING INTO DEEP SLEEP**

When the player is off or asleep, tapping the power button sends the power-on command. When it is on, tapping the power button opens the **Power Options** dialog with **Sleep** and **Deep Sleep** choices.

## Troubleshooting

| Problem | What you see | What to do |
| --- | --- | --- |
| Bluetooth is off | A toast asks you to enable Bluetooth. | Turn on Bluetooth in Android settings, then scan again. |
| Permission was denied | **Bluetooth permissions required** appears. | Grant Bluetooth access in app settings. On Android 6-11 also allow Location for BLE scanning. |
| No player is found | The list remains empty. | Confirm the board is powered on, Bluetooth is enabled, and the phone is nearby. Scan again. |
| Connection fails | The app stays disconnected or shows an error. | Move closer and confirm the board has the current BLE firmware running. |
| A command has no visible effect | The board is off or still changing state. | Wait for the power state to settle, then retry the command. |
| The library is empty | No entries are shown after opening Library. | Confirm the player has a configured music library and the connection is still active. |
| Search returns nothing | **No results found** appears. | Check the search text and selected type. Matching is controlled by the connected player. |
| The queue does not refresh | The list seems stale after a save/load/clear action. | Return to the queue screen and let the app re-request the current queue. |
| Playlist save or delete fails | The app shows a failure message or a retry prompt. | Retry the operation. If it keeps failing, the player/server may not support that playlist action. |

## Frequently Asked Questions

**How do I play a single track?**  
Open **Library**, navigate to the track, and tap it. It is added to the queue. Open **Current Playlist**, tap the track, and choose **Play Now** if it is not already playing.

**How do I play an album?**  
Search by **Album**, then tap the album heading and choose **Add Album to Playlist** or **Replace Playlist with Album**. Then open the queue and use **Play Now** on the desired track.

**How do I search for an artist?**  
Open **Library**, tap **Search**, choose **Artist**, enter the text, and tap **OK**.

**How do I return to a previous screen?**  
Use the toolbar back arrow. In a library folder, use the **Up** row to move upward without leaving the library browser.

**Does the phone need a Wi‑Fi connection?**  
Not for the control link. The app uses BLE to talk to the board and uses moOde HTTP for album-art metadata fetches.

**Does the app play music through the phone?**  
No. The connected audio system plays the music.

**Why is there no general Settings screen?**  
The current app keeps Bluetooth permission handling in Android and uses the gear icon specifically for the moOde host address.

**Can I use the app without the board?**  
The normal app requires a live BLE connection. Development builds may expose a preview path, but that is not part of the standard user workflow.

## Glossary

| Term | Meaning |
| --- | --- |
| **BLE** | Bluetooth Low Energy, the short-range radio connection used between the phone and TinyControlBoard. |
| **Board** | The TinyControlBoard hardware connected to the audio system. |
| **Library** | The folders, tracks, and entries supplied by the connected player. |
| **Queue** | The current playlist or active playback queue known to the board. |
| **Saved playlist** | A named playlist stored by the connected player/server for later loading. |
| **moOde** | The player software that supplies song metadata, artwork, and playlist support for the app. |
| **Track** | One playable music item. |
| **Album** | A group of tracks identified by album metadata. |
| **DAC** | Digital-to-analog converter, the configured output used by the player. |
| **Player display** | The display attached to the audio system that the board controls. |

## Technical Information

This section is for installers and maintainers needing compatibility details.

- Application name: **DanStreamer**
- Android application ID: `com.tinycb.remote`
- Minimum Android version: Android 6.0 (API 23)
- Target Android version: API 34
- Required hardware feature: Bluetooth Low Energy
- Screen orientation: portrait
- Connection model: one active BLE connection to a TinyControlBoard
- moOde metadata path: HTTP from the configured host, separate from BLE control traffic
- Android 12 and later Bluetooth permissions: `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT`
- Android 6-11 Bluetooth permissions: `BLUETOOTH`, `BLUETOOTH_ADMIN`, and `ACCESS_FINE_LOCATION`

The app exchanges commands and status notifications with the board over BLE. Music names, album metadata, queue contents, and search results come from the connected player. Cover art and current-song metadata are fetched from moOde over HTTP, which is why the app has a separate moOde settings dialog.

### Screenshot checklist

Before publishing a final version of this manual, capture:

1. the scan screen with a discovered TinyControlBoard;
2. the connected main panel with album art and now-playing line;
3. the moOde IP settings dialog;
4. the display-view selection screen;
5. the library browser and opened folder view;
6. the search dialog and search-results screen;
7. the current-queue screen with track actions;
8. the saved-playlist management screen;
9. the save-playlist dialog with validation;
10. the power options dialog.

Use portrait screenshots and captions that match the current app layout, not the older UI mockup or the earlier preview-build screens.

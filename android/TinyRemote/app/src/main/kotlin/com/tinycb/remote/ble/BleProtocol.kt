package com.tinycb.remote.ble

import com.tinycb.remote.model.BoardStatus
import com.tinycb.remote.model.LibraryEntry

/**
 * Centralizes all BLE protocol definitions, command IDs, and byte parsing logic.
 */
object BleProtocol {

    // ── Commands ────────────────────────────────────────────────────────────
    const val CMD_BROWSE_REQUEST = 0x0128
    const val CMD_ADD_TRACK = 0x0129
    const val CMD_PLAYLIST_REQUEST = 0x012A
    const val CMD_PLAY_TRACK = 0x012B
    const val CMD_REMOVE_TRACK = 0x012C
    const val CMD_ADD_FOLDER = 0x012D
    const val CMD_REPLACE_WITH_FOLDER = 0x012E
    const val CMD_PLAYLIST_LIST_REQUEST = 0x012F
    const val CMD_PLAYLIST_SAVE = 0x0130
    const val CMD_PLAYLIST_SAVE_OVERWRITE = 0x0131
    const val CMD_PLAYLIST_LOAD = 0x0132
    const val CMD_PLAYLIST_DELETE = 0x0133
    const val CMD_CLEAR_QUEUE = 0x0134
    const val CMD_LIBRARY_SEARCH_ARTIST = 0x0135
    const val CMD_LIBRARY_SEARCH_ALBUM = 0x0136
    const val CMD_LIBRARY_SEARCH_ANY = 0x0137
    const val CMD_ADD_SEARCH_RESULT = 0x0138
    const val LIB_BROWSE_UP = 0xFFFE
    const val LIB_BROWSE_ROOT = 0xFFFF

    // ── Library Constants ────────────────────────────────────────────
    const val LIBRARY_ENTRY_FOLDER = 0
    const val LIBRARY_ENTRY_TRACK = 1
    const val LIBRARY_ENTRY_EMPTY = 2
    const val LIBRARY_ENTRY_RADIO = 3

    data class PlaylistOpResult(val ok: Boolean, val message: String)

    // ── Power States ────────────────────────────────────────────────────────
    fun getPowerStateName(ordinal: Int): String = when (ordinal) {
        0 -> "OFF"
        1 -> "SHUTTING DOWN"
        2 -> "ON"
        3 -> "TURNING ON"
        4 -> "SLEEP"
        5 -> "GOING TO SLEEP"
        6 -> "DEEP SLEEP"
        7 -> "GOING INTO DEEP SLEEP"
        else -> "UNKNOWN"
    }

    fun isTransitioning(state: String?): Boolean = when (state) {
        "SHUTTING DOWN", "TURNING ON", "GOING TO SLEEP", "GOING INTO DEEP SLEEP" -> true
        else -> false
    }

    // ── Parsing ─────────────────────────────────────────────────────────────

    fun parseStatus(value: ByteArray, currentStatus: BoardStatus?): BoardStatus? {
        if (value.size < 3) return currentStatus
        val powerStateOrdinal = value[0].toInt() and 0xFF
        val bitmask = ((value[2].toInt() and 0xFF) shl 8) or (value[1].toInt() and 0xFF)
        val powerName = getPowerStateName(powerStateOrdinal)

        return BoardStatus(
            powerStateName = powerName,
            buttonLedBitmask = bitmask,
            nowPlaying = currentStatus?.nowPlaying,
            trackElapsedSec = currentStatus?.trackElapsedSec ?: 0,
            trackDurationSec = currentStatus?.trackDurationSec ?: 0,
            isTrackPlaying = currentStatus?.isTrackPlaying ?: false,
            trackProgressUpdatedAtMs = currentStatus?.trackProgressUpdatedAtMs ?: 0L
        )
    }

    fun parseNowPlaying(value: ByteArray, currentStatus: BoardStatus?): BoardStatus? {
        val text = value.toTrimmedString()
        return currentStatus?.copy(nowPlaying = text.ifEmpty { null })
    }

    fun parseTrackProgress(value: ByteArray, currentStatus: BoardStatus?): BoardStatus? {
        if (value.size != 5) return currentStatus
        val elapsed = (value[0].toInt() and 0xFF) or ((value[1].toInt() and 0xFF) shl 8)
        val duration = (value[2].toInt() and 0xFF) or ((value[3].toInt() and 0xFF) shl 8)
        val isPlaying = value[4].toInt() != 0
        
        return currentStatus?.copy(
            trackElapsedSec = elapsed,
            trackDurationSec = duration,
            isTrackPlaying = isPlaying,
            trackProgressUpdatedAtMs = System.currentTimeMillis()
        )
    }

    fun parseLibraryEntry(value: ByteArray, currentListing: List<LibraryEntry>): List<LibraryEntry>? {
        if (value.size < 6) return null
        val entryType = value[0].toInt() and 0xFF
        val index = (value[1].toInt() and 0xFF) or ((value[2].toInt() and 0xFF) shl 8)
        val total = (value[3].toInt() and 0xFF) or ((value[4].toInt() and 0xFF) shl 8)

        if (entryType == LIBRARY_ENTRY_EMPTY) {
            return emptyList()
        }

        val nameLen = (value[5].toInt() and 0xFF).coerceAtMost(value.size - 6)
        val name = value.copyOfRange(6, 6 + nameLen).toTrimmedString()
        val album = value.copyOfRange(6 + nameLen, value.size).toTrimmedString()
        val entry = LibraryEntry(
            index = index,
            total = total,
            isDirectory = entryType == LIBRARY_ENTRY_FOLDER,
            name = name,
            isRadioStation = entryType == LIBRARY_ENTRY_RADIO,
            albumName = album
        )
        
        return if (index == 0) listOf(entry) else currentListing + entry
    }

    /** Parses a PLAYLIST_RESULT_CHAR notification: MSG_PLAYLIST_RESULT payload = [ok(u8), message]. */
    fun parsePlaylistResult(value: ByteArray): PlaylistOpResult? {
        if (value.isEmpty()) return null
        val ok = value[0].toInt() != 0
        val message = value.copyOfRange(1, value.size).toTrimmedString()
        return PlaylistOpResult(ok, message)
    }

    private fun ByteArray.toTrimmedString(): String {
        val s = this.toString(Charsets.UTF_8)
        val nullIndex = s.indexOf('\u0000')
        return if (nullIndex != -1) s.substring(0, nullIndex).trim() else s.trim()
    }
}

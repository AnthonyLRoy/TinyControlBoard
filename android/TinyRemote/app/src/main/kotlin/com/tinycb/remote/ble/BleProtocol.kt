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
    const val LIB_BROWSE_UP = 0xFFFE
    const val LIB_BROWSE_ROOT = 0xFFFF

    // ── Library Constants ────────────────────────────────────────────
    const val LIBRARY_ENTRY_FOLDER = 0
    const val LIBRARY_ENTRY_TRACK = 1
    const val LIBRARY_ENTRY_EMPTY = 2

    // ── Playlist operation result (reuses the MSG_LIBRARY_ENTRY wire format with a
    //    sentinel index/total pair to mark "this is a save/load/delete result, not a listing row") ──
    const val LIBRARY_ENTRY_PLAYLIST_OK = 3
    const val LIBRARY_ENTRY_PLAYLIST_ERROR = 4
    const val PLAYLIST_RESULT_SENTINEL = 0xFFFF

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
        if (value.size < 5) return null
        val entryType = value[0].toInt() and 0xFF
        val index = (value[1].toInt() and 0xFF) or ((value[2].toInt() and 0xFF) shl 8)
        val total = (value[3].toInt() and 0xFF) or ((value[4].toInt() and 0xFF) shl 8)

        if (entryType == LIBRARY_ENTRY_EMPTY) {
            return emptyList()
        }

        val name = value.copyOfRange(5, value.size).toTrimmedString()
        val entry = LibraryEntry(index, total, isDirectory = entryType == LIBRARY_ENTRY_FOLDER, name = name)
        
        return if (index == 0) listOf(entry) else currentListing + entry
    }

    /** Returns non-null only when [value] is a playlist save/load/delete result sentinel
     *  (not a normal listing row). */
    fun parsePlaylistResult(value: ByteArray): PlaylistOpResult? {
        if (value.size < 5) return null
        val entryType = value[0].toInt() and 0xFF
        if (entryType != LIBRARY_ENTRY_PLAYLIST_OK && entryType != LIBRARY_ENTRY_PLAYLIST_ERROR) return null
        val index = (value[1].toInt() and 0xFF) or ((value[2].toInt() and 0xFF) shl 8)
        val total = (value[3].toInt() and 0xFF) or ((value[4].toInt() and 0xFF) shl 8)
        if (index != PLAYLIST_RESULT_SENTINEL || total != PLAYLIST_RESULT_SENTINEL) return null

        val message = value.copyOfRange(5, value.size).toTrimmedString()
        return PlaylistOpResult(ok = entryType == LIBRARY_ENTRY_PLAYLIST_OK, message = message)
    }

    private fun ByteArray.toTrimmedString(): String {
        val s = this.toString(Charsets.UTF_8)
        val nullIndex = s.indexOf('\u0000')
        return if (nullIndex != -1) s.substring(0, nullIndex).trim() else s.trim()
    }
}

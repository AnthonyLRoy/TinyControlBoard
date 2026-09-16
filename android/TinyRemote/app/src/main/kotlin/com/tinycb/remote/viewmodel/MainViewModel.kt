package com.tinycb.remote.viewmodel

import android.app.Application
import android.graphics.Bitmap
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.tinycb.remote.R
import com.tinycb.remote.ble.BoardBleManagerHolder
import com.tinycb.remote.ble.ConnectionState
import com.tinycb.remote.data.ButtonCatalog
import com.tinycb.remote.model.BoardStatus
import com.tinycb.remote.model.GridItem
import com.tinycb.remote.model.LibraryEntry
import com.tinycb.remote.net.CoverArtFetcher
import com.tinycb.remote.net.MoodeSettings
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import com.tinycb.remote.ble.BleProtocol
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.launch

class MainViewModel(application: Application) : AndroidViewModel(application) {

    // Shared singleton — ScanActivity and MainActivity must talk to the SAME
    // BLE connection, not two independently-connected instances.
    val bleManager = BoardBleManagerHolder.get(application)

    val connectionState: StateFlow<ConnectionState> = bleManager.connectionState
    val boardStatus: StateFlow<BoardStatus?> = bleManager.status
    val nowPlaying: StateFlow<String?> = boardStatus.map { it?.nowPlaying }
        .stateIn(viewModelScope, SharingStarted.Lazily, null)

    // Fetched separately over HTTP from moOde — BLE never carries image data.
    private val _albumArt = MutableStateFlow<Bitmap?>(null)
    val albumArt: StateFlow<Bitmap?> = _albumArt.asStateFlow()

    init {
        viewModelScope.launch {
            nowPlaying.collectLatest { track ->
                _albumArt.value = if (track.isNullOrEmpty()) null
                    else CoverArtFetcher.fetchCoverArt(MoodeSettings.getHost(application))
            }
        }
    }

    /** Re-fetches album art for the current track, e.g. after the moOde host was changed. */
    fun refreshAlbumArt() {
        val track = nowPlaying.value
        viewModelScope.launch {
            _albumArt.value = if (track.isNullOrEmpty()) null
                else CoverArtFetcher.fetchCoverArt(MoodeSettings.getHost(getApplication()))
        }
    }

    val buttons: StateFlow<List<GridItem>> = boardStatus.combine(MutableStateFlow(ButtonCatalog.gridItems)) { status: BoardStatus?, items: List<GridItem> ->
        items.map { item ->
            if (item !is GridItem.Button) return@map item
            val def = item.def
            when (def.commandId) {
                ButtonCatalog.CMD_DISPLAY_OFF -> {
                    // Display LED bit is inverted relative to desired UI semantics.
                    // We treat LED active as display OFF, and inactive as display ON.
                    val rawLedOn = status != null && def.bitmaskBit >= 0 &&
                        (status.buttonLedBitmask and (1 shl def.bitmaskBit)) != 0
                    val isOn = !rawLedOn
                    GridItem.Button(def.copy(
                        name = if (isOn) "Display On" else "Display Off"
                    ))
                }
                ButtonCatalog.CMD_PLAY -> {
                    // Play → Pause (icon + label) when track is playing
                    val isPlaying = status?.isTrackPlaying == true
                    GridItem.Button(
                        def.copy(
                            name    = if (isPlaying) "Pause" else "Play",
                            iconRes = if (isPlaying) R.drawable.ic_play_pause else R.drawable.ic_play
                        )
                    )
                }
                else -> item
            }

        }
    }.stateIn(viewModelScope, SharingStarted.Lazily, ButtonCatalog.gridItems)

    val selectedViewId: StateFlow<Int?> = bleManager.selectedViewId

    // ── Library (folder browse + current queue) ─────────────────────────────
    inner class LibraryController {
        val listing: StateFlow<List<LibraryEntry>> = bleManager.libraryListing
        fun browseRoot() = bleManager.browseRoot()
        fun browseUp() = bleManager.browseUp()
        fun browseInto(index: Int) = bleManager.browseInto(index)
        fun addTrack(index: Int) = bleManager.addTrack(index)
        fun addFolder(index: Int) = bleManager.addFolder(index)
        fun replaceWithFolder(index: Int) = bleManager.replaceWithFolder(index)
        fun playTrack(index: Int) = bleManager.playTrack(index)
        fun removeTrack(index: Int) = bleManager.removeTrack(index)
        fun moveTrack(from: Int, to: Int) = bleManager.moveTrack(from, to)
        fun requestQueue() = bleManager.requestPlaylist()
        fun clearQueue() = bleManager.clearQueue()
    }
    val library = LibraryController()

    // ── Library search (Artist/Album/Any) ────────────────────────────────────
    inner class SearchController {
        val results: StateFlow<List<LibraryEntry>?> = bleManager.searchResults
        fun searchArtist(text: String) = bleManager.searchArtist(text)
        fun searchAlbum(text: String) = bleManager.searchAlbum(text)
        fun searchAny(text: String) = bleManager.searchAny(text)
        fun addResult(index: Int) = bleManager.addSearchResult(index)
        fun clear() = bleManager.clearSearchResults()
    }
    val search = SearchController()

    // ── Saved playlists (save/load/delete) ───────────────────────────────────
    inner class PlaylistController {
        val nameEntries: StateFlow<List<LibraryEntry>?> = bleManager.playlistNameEntries
        val opResult: StateFlow<BleProtocol.PlaylistOpResult?> = bleManager.playlistOpResult
        fun requestNames() = bleManager.requestPlaylistNames()
        fun clearOpResult() = bleManager.clearPlaylistOpResult()
        fun save(name: String) = bleManager.savePlaylist(name)
        fun overwrite(name: String) = bleManager.overwritePlaylist(name)
        fun load(name: String) = bleManager.loadPlaylist(name)
        fun delete(name: String) = bleManager.deletePlaylist(name)
    }
    val playlist = PlaylistController()

    // ── Track Progress State ────────────────────────────────────────────────
    data class ProgressState(
        val elapsed: Int = 0,
        val duration: Int = 0,
        val isVisible: Boolean = false,
        val lastUpdateMs: Long = 0L,
        val lastPingMs: Long = 0L
    )

    private val _trackProgress = MutableStateFlow(ProgressState())
    val trackProgress = _trackProgress.asStateFlow()

    private var progressTickerJob: Job? = null

    /** Tap-to-seek on the progress bar: [percent] is 0-100 of the current track's duration.
     *  Updates local state immediately for a responsive UI, then sends the seek command. */
    fun seekToPercent(percent: Int) {
        val clamped = percent.coerceIn(0, 100)
        val current = _trackProgress.value
        if (current.duration > 0) {
            _trackProgress.value = current.copy(
                elapsed = (current.duration * clamped) / 100,
                lastUpdateMs = System.currentTimeMillis()
            )
        }
        viewModelScope.launch(Dispatchers.IO) {
            bleManager.seekToPercent(clamped)
        }
    }

    // ── Power State Logic ───────────────────────────────────────────────────
    private val _isPowerFlashing = MutableStateFlow(false)
    val isPowerFlashing = _isPowerFlashing.asStateFlow()

    private val _powerState = MutableStateFlow<String?>(null)
    val powerState = _powerState.asStateFlow()

    private var isManuallyFlashing = false
    private var powerStateAtClick: String? = null
    private var manualFlashJob: Job? = null

    init {
        // Observe board status to update ViewModel internal states
        viewModelScope.launch {
            boardStatus.collectLatest { status ->
                val stateName = status?.powerStateName
                
                // Track Progress Update
                _trackProgress.value = ProgressState(
                    elapsed = status?.trackElapsedSec ?: 0,
                    duration = status?.trackDurationSec ?: 0,
                    isVisible = status != null,
                    lastUpdateMs = status?.trackProgressUpdatedAtMs ?: 0L
                )
                
                // Ticker logic
                if (status?.isTrackPlaying == true) {
                    startTicker()
                } else {
                    stopTicker()
                }

                // Power state logic
                handlePowerStateUpdate(stateName)
                _powerState.value = stateName
            }
        }
    }

    private fun startTicker() {
        if (progressTickerJob?.isActive == true) return
        progressTickerJob = viewModelScope.launch {
            while (true) {
                delay(500)
                // Update lastPingMs to force flow emission, triggering UI interpolation
                _trackProgress.update { it.copy(lastPingMs = System.currentTimeMillis()) }
            }
        }
    }

    private fun stopTicker() {
        progressTickerJob?.cancel()
        progressTickerJob = null
    }

    private fun handlePowerStateUpdate(stateName: String?) {
        // Confirm manual flash completion
        if (isManuallyFlashing && stateName != null && !BleProtocol.isTransitioning(stateName)) {
            if (stateName != powerStateAtClick) {
                isManuallyFlashing = false
                manualFlashJob?.cancel()
                powerStateAtClick = null
            }
        }
        
        val transitioning = BleProtocol.isTransitioning(stateName) || isManuallyFlashing
        _isPowerFlashing.value = transitioning
    }

    /** True when the board is fully ON, i.e. a power-button press would trigger Sleep/DeepSleep
     *  on the firmware rather than a power-on request — this is when we must ask the user
     *  which kind of sleep they want. */
    fun isPoweredOn(): Boolean = _powerState.value == "ON"

    fun onPowerClicked() = sendPowerCommand(releaseMs = 0) // short press → Sleep (or PowerOn if not ON)

    fun onDeepSleepClicked() = sendPowerCommand(releaseMs = DEEP_SLEEP_RELEASE_MS) // long press → DeepSleep

    private fun sendPowerCommand(releaseMs: Int) {
        isManuallyFlashing = true
        powerStateAtClick = _powerState.value
        _isPowerFlashing.value = true
        viewModelScope.launch(Dispatchers.IO) {
            bleManager.sendCommand(0x0001, releaseMs) // POWER_COMMAND_ID
        }

        manualFlashJob?.cancel()
        manualFlashJob = viewModelScope.launch {
            delay(4000)
            if (isManuallyFlashing) {
                isManuallyFlashing = false
                _isPowerFlashing.value = BleProtocol.isTransitioning(_powerState.value)
            }
        }
    }

    fun setSelectedView(commandId: Int) {
        bleManager.setSelectedViewId(commandId)
        sendCommand(commandId)
    }

    fun toggleDisplay() {
        sendCommand(0x0114)
    }

    fun sendCommand(commandId: Int) {
        viewModelScope.launch(Dispatchers.IO) {
            bleManager.sendCommand(commandId)
        }
    }

    override fun onCleared() {
        super.onCleared()
    }

    companion object {
        // Must exceed the firmware's kLongPressThresholdMs (3000ms) so CMD_SYS_POWER
        // is classified as a DeepSleep transition instead of a plain Sleep.
        private const val DEEP_SLEEP_RELEASE_MS = 3500
    }
}

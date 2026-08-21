package com.tinycb.remote.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.tinycb.remote.R
import com.tinycb.remote.ble.BoardBleManagerHolder
import com.tinycb.remote.ble.ConnectionState
import com.tinycb.remote.data.ButtonCatalog
import com.tinycb.remote.model.BoardStatus
import com.tinycb.remote.model.GridItem
import com.tinycb.remote.model.LibraryEntry
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
    
    val buttons: StateFlow<List<GridItem>> = boardStatus.combine(MutableStateFlow(ButtonCatalog.gridItems)) { status: BoardStatus?, items: List<GridItem> ->
        items.map { item ->
            if (item !is GridItem.Button) return@map item
            val def = item.def
            when (def.commandId) {
                ButtonCatalog.CMD_DISPLAY_OFF -> {
                    // Display Off → Display On when LED active (internally, but we don't show the label)
                    val isOn = status != null && def.bitmaskBit >= 0 &&
                        (status.buttonLedBitmask and (1 shl def.bitmaskBit)) != 0
                    GridItem.Button(def.copy(
                        name = if (isOn) "Display On" else "Display Off",
                        showLabel = false
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

    val libraryListing: StateFlow<List<LibraryEntry>> = bleManager.libraryListing

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

    fun onPowerClicked() {
        isManuallyFlashing = true
        powerStateAtClick = _powerState.value
        _isPowerFlashing.value = true
        sendCommand(0x0001) // POWER_COMMAND_ID

        manualFlashJob?.cancel()
        manualFlashJob = viewModelScope.launch {
            delay(4000)
            if (isManuallyFlashing) {
                isManuallyFlashing = false
                _isPowerFlashing.value = BleProtocol.isTransitioning(_powerState.value)
            }
        }
    }

    fun browseRoot() = bleManager.browseRoot()
    fun browseUp() = bleManager.browseUp()
    fun browseInto(index: Int) = bleManager.browseInto(index)
    fun addTrack(index: Int) = bleManager.addTrack(index)
    fun playTrack(index: Int) = bleManager.playTrack(index)
    fun requestPlaylist() = bleManager.requestPlaylist()

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
}

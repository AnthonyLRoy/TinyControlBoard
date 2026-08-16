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
import kotlinx.coroutines.flow.combine
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
    
    val buttons: StateFlow<List<GridItem>> = boardStatus.combine(MutableStateFlow(ButtonCatalog.gridItems)) { status, items ->
        items.map { item ->
            if (item !is GridItem.Button) return@map item
            val def = item.def
            when (def.commandId) {
                ButtonCatalog.CMD_DISPLAY_OFF -> {
                    // Display Off → Display On when LED active
                    val isOn = status != null && def.bitmaskBit >= 0 &&
                        (status.buttonLedBitmask and (1 shl def.bitmaskBit)) != 0
                    GridItem.Button(def.copy(name = if (isOn) "Display On" else "Display Off"))
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

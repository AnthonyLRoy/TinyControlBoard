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
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.launch

class MainViewModel(application: Application) : AndroidViewModel(application) {

    // Shared singleton — ScanActivity and MainActivity must talk to the SAME
    // BLE connection, not two independently-connected instances.
    val bleManager = BoardBleManagerHolder.get(application)

    val connectionState: StateFlow<ConnectionState> = bleManager.connectionState
    val boardStatus: StateFlow<BoardStatus?> = bleManager.status
    
    val buttons: StateFlow<List<GridItem>> = boardStatus.combine(MutableStateFlow(ButtonCatalog.gridItems)) { status, items ->
        items.map { item ->
            if (item !is GridItem.Button) return@map item
            val def = item.def
            when (def.commandId) {
                0x0114 -> {
                    // Display Off → Display On when LED active
                    val isOn = status != null && def.bitmaskBit >= 0 &&
                        (status.buttonLedBitmask and (1 shl def.bitmaskBit)) != 0
                    GridItem.Button(def.copy(name = if (isOn) "Display On" else "Display Off"))
                }
                0x0102 -> {
                    // Play → Pause (icon + label) when the play LED is active
                    val isPlaying = status != null && def.bitmaskBit >= 0 &&
                        (status.buttonLedBitmask and (1 shl def.bitmaskBit)) != 0
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

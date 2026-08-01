package com.tinycb.remote.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.tinycb.remote.ble.BoardBleManagerHolder
import com.tinycb.remote.ble.ConnectionState
import com.tinycb.remote.data.ButtonCatalog
import com.tinycb.remote.model.BoardStatus
import com.tinycb.remote.model.ButtonDef
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
    
    val buttons: StateFlow<List<ButtonDef>> = boardStatus.combine(MutableStateFlow(ButtonCatalog.buttons)) { status, allButtons ->
        allButtons.map { btn ->
            if (btn.commandId == 0x0114) {
                val isOn = status != null && (status.buttonLedBitmask and (1 shl btn.bitmaskBit)) != 0
                btn.copy(name = if (isOn) "Display On" else "Display Off")
            } else {
                btn
            }
        }
    }.stateIn(viewModelScope, SharingStarted.Lazily, ButtonCatalog.buttons)

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

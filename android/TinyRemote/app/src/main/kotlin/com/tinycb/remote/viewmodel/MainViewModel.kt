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
    
    private val _isDisplayOn = MutableStateFlow(false)
    val isDisplayOn: StateFlow<Boolean> = _isDisplayOn

    val buttons: StateFlow<List<ButtonDef>> = _isDisplayOn.combine(MutableStateFlow(ButtonCatalog.buttons)) { isOn, allButtons ->
        allButtons.map { btn ->
            if (btn.commandId == 0x0114) {
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
        android.util.Log.d("MainViewModel", "toggleDisplay: current state = ${_isDisplayOn.value}")
        _isDisplayOn.value = !_isDisplayOn.value
        android.util.Log.d("MainViewModel", "toggleDisplay: new state = ${_isDisplayOn.value}")
        sendCommand(0x0114)
    }

    fun sendCommand(commandId: Int) {
        viewModelScope.launch(Dispatchers.IO) {
            bleManager.sendCommand(commandId)
        }
    }

    override fun onCleared() {
        super.onCleared()
        bleManager.disconnect()
    }
}

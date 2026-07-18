package com.tinycb.remote.ble

import android.bluetooth.BluetoothDevice

sealed class ConnectionState {
    object Idle : ConnectionState()
    object Scanning : ConnectionState()
    data class DevicesFound(val devices: List<BluetoothDevice>) : ConnectionState()
    data class Connecting(val device: BluetoothDevice) : ConnectionState()
    object Connected : ConnectionState()
    object Disconnected : ConnectionState()
    data class Error(val message: String) : ConnectionState()
}

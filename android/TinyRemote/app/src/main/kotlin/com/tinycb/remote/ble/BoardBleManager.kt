package com.tinycb.remote.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.os.Build
import android.os.ParcelUuid
import android.util.Log
import com.tinycb.remote.model.BoardStatus
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

/**
 * Manages BLE scan, connection, GATT operations, and status notifications
 * for the TinyControlBoard peripheral.
 *
 * All GATT callbacks arrive on a Binder thread; StateFlow updates are
 * thread-safe by design.
 */
@SuppressLint("MissingPermission")
class BoardBleManager(context: Context) {

    private val appContext = context.applicationContext
    private val bluetoothManager =
        appContext.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val adapter: BluetoothAdapter? get() = bluetoothManager.adapter

    // ── Public state ────────────────────────────────────────────────────────
    private val _connectionState = MutableStateFlow<ConnectionState>(ConnectionState.Idle)
    val connectionState: StateFlow<ConnectionState> = _connectionState

    private val _status = MutableStateFlow<BoardStatus?>(null)
    val status: StateFlow<BoardStatus?> = _status

    // ── Internal state ──────────────────────────────────────────────────────
    private val discoveredDevices = mutableListOf<BluetoothDevice>()
    private var leScanner: BluetoothLeScanner? = null
    private var gatt: BluetoothGatt? = null
    private var cmdChar: BluetoothGattCharacteristic? = null

    // ── Scan callback ───────────────────────────────────────────────────────
    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            if (discoveredDevices.none { it.address == device.address }) {
                discoveredDevices.add(device)
                _connectionState.value = ConnectionState.DevicesFound(discoveredDevices.toList())
                Log.d(TAG, "Found device: ${device.address}")
            }
        }
        override fun onScanFailed(errorCode: Int) {
            Log.e(TAG, "Scan failed: $errorCode")
            _connectionState.value = ConnectionState.Error("BLE scan failed (code $errorCode)")
        }
    }

    // ── GATT callback ───────────────────────────────────────────────────────
    private val gattCallback = object : BluetoothGattCallback() {

        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    Log.i(TAG, "GATT connected; discovering services…")
                    _connectionState.value = ConnectionState.Connected
                    g.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    Log.i(TAG, "GATT disconnected (status=$status)")
                    gatt?.close()
                    gatt = null
                    cmdChar = null
                    _connectionState.value = ConnectionState.Disconnected
                    _status.value = null
                }
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                Log.e(TAG, "Service discovery failed: $status")
                return
            }
            val svc = g.getService(BleUuids.SERVICE)
            if (svc == null) {
                Log.e(TAG, "TinyControlBoard service not found")
                return
            }
            cmdChar = svc.getCharacteristic(BleUuids.CMD_CHAR)
            val statusChar = svc.getCharacteristic(BleUuids.STATUS_CHAR) ?: run {
                Log.e(TAG, "Status characteristic not found")
                return
            }

            // Subscribe to status notifications
            g.setCharacteristicNotification(statusChar, true)
            val cccd = statusChar.getDescriptor(BleUuids.CCCD)
            if (cccd != null) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    g.writeDescriptor(cccd, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                } else {
                    @Suppress("DEPRECATION")
                    cccd.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    @Suppress("DEPRECATION")
                    g.writeDescriptor(cccd)
                }
            }
            Log.i(TAG, "Services set up; notifications enabled")
        }

        // Android 13+ (API 33) overload — preferred
        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray
        ) {
            if (characteristic.uuid == BleUuids.STATUS_CHAR) parseStatus(value)
        }

        // Android < 13 fallback
        @Suppress("DEPRECATION")
        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU
                && characteristic.uuid == BleUuids.STATUS_CHAR) {
                parseStatus(characteristic.value ?: return)
            }
        }
    }

    // ── Helpers ─────────────────────────────────────────────────────────────
    private fun parseStatus(value: ByteArray) {
        if (value.size < 3) return
        val powerStateOrdinal = value[0].toInt() and 0xFF
        val bitmask = ((value[2].toInt() and 0xFF) shl 8) or (value[1].toInt() and 0xFF)
        val powerName = when (powerStateOrdinal) {
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
        _status.value = BoardStatus(powerName, bitmask)
    }

    // ── Public API ───────────────────────────────────────────────────────────
    fun startScan() {
        discoveredDevices.clear()
        _connectionState.value = ConnectionState.Scanning

        val filters = listOf(
            ScanFilter.Builder()
                .setServiceUuid(ParcelUuid(BleUuids.SERVICE))
                .build()
        )
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        leScanner = adapter?.bluetoothLeScanner
        leScanner?.startScan(filters, settings, scanCallback)
            ?: run { _connectionState.value = ConnectionState.Error("Bluetooth not available") }
    }

    fun stopScan() {
        leScanner?.stopScan(scanCallback)
        leScanner = null
    }

    fun connect(device: BluetoothDevice) {
        stopScan()
        _connectionState.value = ConnectionState.Connecting(device)
        gatt = device.connectGatt(appContext, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
    }

    @SuppressLint("MissingPermission")
    fun sendCommand(commandId: Int) {
        val char = cmdChar ?: return
        val g    = gatt    ?: return
        val bytes = byteArrayOf(
            (commandId and 0xFF).toByte(),
            ((commandId shr 8) and 0xFF).toByte()
        )
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            g.writeCharacteristic(char, bytes, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
        } else {
            @Suppress("DEPRECATION")
            char.value = bytes
            @Suppress("DEPRECATION")
            char.writeType = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            @Suppress("DEPRECATION")
            g.writeCharacteristic(char)
        }
    }

    fun disconnect() {
        stopScan()
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        cmdChar = null
        _connectionState.value = ConnectionState.Idle
        _status.value = null
    }

    companion object {
        private const val TAG = "BoardBleManager"
    }
}

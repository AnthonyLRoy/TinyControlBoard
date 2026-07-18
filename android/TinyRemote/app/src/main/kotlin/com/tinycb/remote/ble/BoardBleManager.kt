package com.tinycb.remote.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
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
    @Volatile private var gatt: BluetoothGatt? = null
    @Volatile private var cmdChar: BluetoothGattCharacteristic? = null
    private val mainHandler = Handler(Looper.getMainLooper())
    private val connectTimeoutMs = 15_000L
    private val connectTimeoutRunnable = Runnable {
        if (_connectionState.value is ConnectionState.Connecting) {
            Log.w(TAG, "Connection timed out after ${connectTimeoutMs}ms")
            gatt?.close()
            gatt = null
            _connectionState.value = ConnectionState.Error(
                "Connection timed out.\nMake sure the board is powered on and BLE firmware is running."
            )
        }
    }

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
            mainHandler.removeCallbacks(connectTimeoutRunnable)
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    Log.i(TAG, "GATT connected (status=$status); discovering services…")
                    // Stay in Connecting — emit Connected only after onServicesDiscovered
                    // confirms both characteristics are ready (prevents null cmdChar race).
                    g.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    Log.w(TAG, "GATT disconnected (status=$status)")
                    gatt?.close()
                    gatt = null
                    cmdChar = null
                    if (status != BluetoothGatt.GATT_SUCCESS) {
                        _connectionState.value = ConnectionState.Error("Connection failed (GATT status $status)")
                    } else {
                        _connectionState.value = ConnectionState.Disconnected
                    }
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
                Log.e(TAG, "TinyControlBoard service not found — UUIDs: ${g.services.map { it.uuid }}")
                return
            }

            // Log all characteristics for diagnosis
            Log.i(TAG, "Service found with ${svc.characteristics.size} characteristics:")
            svc.characteristics.forEach { chr ->
                Log.i(TAG, "  uuid=${chr.uuid}  props=${chr.properties}")
            }

            cmdChar = svc.getCharacteristic(BleUuids.CMD_CHAR)
            if (cmdChar == null) {
                Log.e(TAG, "CMD characteristic NOT found \u2014 refreshing GATT cache and re-discovering")
                refreshGattCache(g)
                g.discoverServices()
                return
            }

            val statusChar = svc.getCharacteristic(BleUuids.STATUS_CHAR) ?: run {
                Log.e(TAG, "Status characteristic not found")
                return
            }

            // Both characteristics ready \u2014 now it is safe to open the control panel
            Log.i(TAG, "All characteristics found \u2014 emitting Connected")
            _connectionState.value = ConnectionState.Connected

            // Subscribe to status notifications
            g.setCharacteristicNotification(statusChar, true)
            val cccd = statusChar.getDescriptor(BleUuids.CCCD)
            if (cccd != null) {
                Log.d(TAG, "Writing CCCD to enable notifications on status char")
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    g.writeDescriptor(cccd, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                } else {
                    @Suppress("DEPRECATION")
                    cccd.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    @Suppress("DEPRECATION")
                    g.writeDescriptor(cccd)
                }
            } else {
                Log.e(TAG, "CCCD descriptor not found on status characteristic!")
            }
            Log.i(TAG, "Services set up; cmdChar and notifications enabled")
        }

        override fun onDescriptorWrite(g: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.i(TAG, "CCCD write confirmed (handle=${descriptor.characteristic?.uuid}) — notifications active")
            } else {
                Log.e(TAG, "CCCD write FAILED status=$status — notifications will not arrive!")
            }
        }

        // Android 13+ (API 33) overload — preferred
        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray
        ) {
            Log.d(TAG, "onCharacteristicChanged (API33+): uuid=${characteristic.uuid} bytes=${value.size}")
            if (characteristic.uuid == BleUuids.STATUS_CHAR) parseStatus(value)
        }

        // Android < 13 fallback
        @Suppress("DEPRECATION", "OVERRIDE_DEPRECATION")
        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            Log.d(TAG, "onCharacteristicChanged (legacy): uuid=${characteristic.uuid}")
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU
                && characteristic.uuid == BleUuids.STATUS_CHAR) {
                parseStatus(characteristic.value ?: return)
            }
        }
    }

    // ── Helpers ─────────────────────────────────────────────────────────────
    /** Clears the Android GATT service cache via the hidden refresh() API.
     *  Silently ignored if the API is unavailable. */
    private fun refreshGattCache(g: BluetoothGatt) {
        try {
            val method = g.javaClass.getMethod("refresh")
            val result = method.invoke(g) as Boolean
            Log.i(TAG, "BluetoothGatt.refresh() = $result — stale service cache cleared")
        } catch (e: Exception) {
            Log.w(TAG, "BluetoothGatt.refresh() unavailable: ${e.message}")
        }
    }

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
        if (gatt == null) {
            Log.e(TAG, "connectGatt returned null — BLUETOOTH_CONNECT permission likely missing")
            _connectionState.value = ConnectionState.Error(
                "Could not start connection.\nGrant Bluetooth permission in Settings and retry."
            )
            return
        }
        // Start timeout watchdog
        mainHandler.postDelayed(connectTimeoutRunnable, connectTimeoutMs)
    }

    @SuppressLint("MissingPermission")
    fun sendCommand(commandId: Int) {
        val char = cmdChar ?: run { Log.w(TAG, "sendCommand 0x%04X: cmdChar is null".format(commandId)); return }
        val g    = gatt    ?: run { Log.w(TAG, "sendCommand 0x%04X: gatt is null".format(commandId)); return }
        Log.d(TAG, "sendCommand: writing 0x%04X to CMD characteristic".format(commandId))
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
        mainHandler.removeCallbacks(connectTimeoutRunnable)
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

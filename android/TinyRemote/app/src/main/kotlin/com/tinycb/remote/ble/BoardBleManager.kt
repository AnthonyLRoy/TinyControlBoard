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
import com.tinycb.remote.model.LibraryEntry
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import java.util.ArrayDeque

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

    private val _selectedViewId = MutableStateFlow<Int?>(null)
    val selectedViewId: StateFlow<Int?> = _selectedViewId

    private val _libraryListing = MutableStateFlow<List<LibraryEntry>>(emptyList())
    val libraryListing: StateFlow<List<LibraryEntry>> = _libraryListing

    private val _playlistNameEntries = MutableStateFlow<List<LibraryEntry>?>(null)
    val playlistNameEntries: StateFlow<List<LibraryEntry>?> = _playlistNameEntries

    private val _playlistOpResult = MutableStateFlow<BleProtocol.PlaylistOpResult?>(null)
    val playlistOpResult: StateFlow<BleProtocol.PlaylistOpResult?> = _playlistOpResult

    // When true, incoming MSG_LIBRARY_ENTRY notifications populate playlistNameEntries
    // instead of libraryListing (see requestPlaylistNames()).
    @Volatile private var playlistNameMode = false

    // ── Internal state ──────────────────────────────────────────────────────
    private val discoveredDevices = mutableListOf<BluetoothDevice>()
    private var leScanner: BluetoothLeScanner? = null
    @Volatile private var gatt: BluetoothGatt? = null
    @Volatile private var cmdChar: BluetoothGattCharacteristic? = null
    @Volatile private var libraryCmdChar: BluetoothGattCharacteristic? = null
    @Volatile private var playlistCmdChar: BluetoothGattCharacteristic? = null
    private val mainHandler = Handler(Looper.getMainLooper())
    private val notificationQueue = ArrayDeque<BluetoothGattCharacteristic>()
    private var notificationWriteInFlight = false
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
                    Log.i(TAG, "GATT connected (status=$status). Requesting MTU…")
                    g.requestMtu(512)
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    Log.w(TAG, "GATT disconnected (status=$status)")
                    gatt?.close()
                    gatt = null
                    cmdChar = null
                    libraryCmdChar = null
                    playlistCmdChar = null
                    clearNotificationQueue()
                    if (status != BluetoothGatt.GATT_SUCCESS) {
                        _connectionState.value = ConnectionState.Error("Connection failed (GATT status $status)")
                    } else {
                        _connectionState.value = ConnectionState.Disconnected
                    }
                    _status.value = null
                    _libraryListing.value = emptyList()
                    _playlistNameEntries.value = null
                    _playlistOpResult.value = null
                    playlistNameMode = false
                }
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.i(TAG, "MTU negotiated: $mtu bytes")
            } else {
                Log.w(TAG, "MTU negotiation failed (status=$status); discovering services with the default MTU")
            }
            g.discoverServices()
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
                Log.e(TAG, "CMD characteristic NOT found — refreshing GATT cache and re-discovering")
                refreshGattCache(g)
                g.discoverServices()
                return
            }

            val statusChar = svc.getCharacteristic(BleUuids.STATUS_CHAR) ?: run {
                Log.e(TAG, "Status characteristic not found")
                return
            }

            val nowPlayingChar = svc.getCharacteristic(BleUuids.NOW_PLAYING_CHAR)
            val trackProgressChar = svc.getCharacteristic(BleUuids.TRACK_PROGRESS_CHAR)
            val libraryChar = svc.getCharacteristic(BleUuids.LIBRARY_CHAR)
            val playlistResultChar = svc.getCharacteristic(BleUuids.PLAYLIST_RESULT_CHAR)
            libraryCmdChar = svc.getCharacteristic(BleUuids.LIBRARY_CMD_CHAR)
            if (libraryCmdChar == null) {
                Log.w(TAG, "Library-cmd characteristic not found — library browse unavailable")
            }
            playlistCmdChar = svc.getCharacteristic(BleUuids.PLAYLIST_CMD_CHAR)
            if (playlistCmdChar == null) {
                Log.w(TAG, "Playlist-cmd characteristic not found — playlist management unavailable")
            }

            // Both required characteristics ready — now it is safe to open the control panel
            Log.i(TAG, "All characteristics found — emitting Connected")
            _connectionState.value = ConnectionState.Connected(g.device.name)

            clearNotificationQueue()
            enqueueNotification(statusChar)
            if (nowPlayingChar != null) {
                enqueueNotification(nowPlayingChar)
            } else {
                Log.w(TAG, "Now-playing characteristic not found — track display unavailable")
            }
            if (trackProgressChar != null) {
                enqueueNotification(trackProgressChar)
            } else {
                Log.w(TAG, "Track-progress characteristic not found — progress bar unavailable")
            }
            if (libraryChar != null) {
                enqueueNotification(libraryChar)
            } else {
                Log.w(TAG, "Library characteristic not found — library browse unavailable")
            }
            if (playlistResultChar != null) {
                enqueueNotification(playlistResultChar)
            } else {
                Log.w(TAG, "Playlist-result characteristic not found — playlist save/load/delete feedback unavailable")
            }
            writeNextNotification(g)

            // Read initial status after a short delay
            mainHandler.postDelayed({
                val sChar = g.getService(BleUuids.SERVICE)?.getCharacteristic(BleUuids.STATUS_CHAR)
                if (sChar != null) {
                    g.readCharacteristic(sChar)
                }
            }, 1000L)
        }

        override fun onDescriptorWrite(g: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.i(TAG, "CCCD write confirmed (handle=${descriptor.characteristic?.uuid}) — notifications active")
            } else {
                Log.e(TAG, "CCCD write FAILED status=$status — notifications will not arrive!")
            }
            notificationWriteInFlight = false
            writeNextNotification(g)
        }

        // Android 13+ (API 33) overload — preferred
        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray
        ) {
            Log.d(TAG, "onCharacteristicChanged (API33+): uuid=${characteristic.uuid} bytes=${value.size}")
            when (characteristic.uuid) {
                BleUuids.STATUS_CHAR         -> parseStatus(value)
                BleUuids.NOW_PLAYING_CHAR    -> parseNowPlaying(value)
                BleUuids.TRACK_PROGRESS_CHAR -> parseTrackProgress(value)
                BleUuids.LIBRARY_CHAR        -> parseLibraryEntry(value)
                BleUuids.PLAYLIST_RESULT_CHAR -> parsePlaylistResult(value)
                else -> Log.w(TAG, "onCharacteristicChanged (API33+): unknown uuid ${characteristic.uuid}")
            }
        }

        @Suppress("DEPRECATION", "OVERRIDE_DEPRECATION")
        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            val value = characteristic.value ?: run {
                Log.w(TAG, "onCharacteristicChanged (legacy): value is null for ${characteristic.uuid}")
                return
            }
            Log.d(TAG, "onCharacteristicChanged (legacy): uuid=${characteristic.uuid} bytes=${value.size}")
            when (characteristic.uuid) {
                BleUuids.STATUS_CHAR         -> parseStatus(value)
                BleUuids.NOW_PLAYING_CHAR    -> parseNowPlaying(value)
                BleUuids.TRACK_PROGRESS_CHAR -> parseTrackProgress(value)
                BleUuids.LIBRARY_CHAR        -> parseLibraryEntry(value)
                BleUuids.PLAYLIST_RESULT_CHAR -> parsePlaylistResult(value)
                else -> Log.w(TAG, "onCharacteristicChanged (legacy): unknown uuid ${characteristic.uuid}")
            }
        }

        // Android 13+ (API 33) overload — preferred
        override fun onCharacteristicRead(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray,
            status: Int
        ) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                Log.w(TAG, "onCharacteristicRead (API33+): failed uuid=${characteristic.uuid} status=$status")
                return
            }
            Log.d(TAG, "onCharacteristicRead (API33+): uuid=${characteristic.uuid} bytes=${value.size}")
            if (characteristic.uuid == BleUuids.STATUS_CHAR) {
                parseStatus(value)
            }
        }

        @Suppress("DEPRECATION", "OVERRIDE_DEPRECATION")
        override fun onCharacteristicRead(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                Log.w(TAG, "onCharacteristicRead (legacy): failed uuid=${characteristic.uuid} status=$status")
                return
            }
            val value = characteristic.value ?: run {
                Log.w(TAG, "onCharacteristicRead (legacy): value is null for ${characteristic.uuid}")
                return
            }
            Log.d(TAG, "onCharacteristicRead (legacy): uuid=${characteristic.uuid} bytes=${value.size}")
            if (characteristic.uuid == BleUuids.STATUS_CHAR) {
                parseStatus(value)
            }
        }
    }

    // ── Helpers ─────────────────────────────────────────────────────────────
    /** Little-endian 2-byte encoding shared by every command/write path. */
    private fun commandId16Bytes(commandId: Int) = byteArrayOf(
        (commandId and 0xFF).toByte(),
        ((commandId shr 8) and 0xFF).toByte()
    )

    /** Writes [bytes] to [characteristic] using the current GATT connection, handling the
     *  API 33+ vs. legacy write API split in one place. No-op (with a log) if not connected. */
    @SuppressLint("MissingPermission")
    private fun writeToCharacteristic(characteristic: BluetoothGattCharacteristic, bytes: ByteArray) {
        val g = gatt ?: run { Log.w(TAG, "write to ${characteristic.uuid}: gatt is null"); return }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            g.writeCharacteristic(characteristic, bytes, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE)
        } else {
            @Suppress("DEPRECATION")
            characteristic.value = bytes
            @Suppress("DEPRECATION")
            characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            @Suppress("DEPRECATION")
            g.writeCharacteristic(characteristic)
        }
    }

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
        _status.value = BleProtocol.parseStatus(value, _status.value)
        Log.e(TAG, "parseStatus: updated powerStateName to ${_status.value?.powerStateName}")
    }

    private fun parseNowPlaying(value: ByteArray) {
        _status.value = BleProtocol.parseNowPlaying(value, _status.value)
    }

    private fun parseTrackProgress(value: ByteArray) {
        _status.value = BleProtocol.parseTrackProgress(value, _status.value)
    }

    private fun parseLibraryEntry(value: ByteArray) {
        if (playlistNameMode) {
            BleProtocol.parseLibraryEntry(value, _playlistNameEntries.value ?: emptyList())?.let {
                _playlistNameEntries.value = it
            }
            return
        }
        BleProtocol.parseLibraryEntry(value, _libraryListing.value)?.let {
            _libraryListing.value = it
        }
    }

    private fun parsePlaylistResult(value: ByteArray) {
        _playlistOpResult.value = BleProtocol.parsePlaylistResult(value)
    }

    private fun enqueueNotification(characteristic: BluetoothGattCharacteristic) {
        notificationQueue.addLast(characteristic)
    }

    private fun clearNotificationQueue() {
        notificationQueue.clear()
        notificationWriteInFlight = false
    }

    private fun writeNextNotification(g: BluetoothGatt) {
        if (notificationWriteInFlight || notificationQueue.isEmpty()) return

        val characteristic = notificationQueue.removeFirst()
        val descriptor = characteristic.getDescriptor(BleUuids.CCCD)
        if (descriptor == null || !g.setCharacteristicNotification(characteristic, true)) {
            Log.e(TAG, "Could not enable notifications for ${characteristic.uuid}")
            writeNextNotification(g)
            return
        }

        notificationWriteInFlight = true
        val writeStarted = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            g.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE) == BluetoothStatusCodes.SUCCESS
        } else {
            @Suppress("DEPRECATION")
            descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
            @Suppress("DEPRECATION")
            g.writeDescriptor(descriptor)
        }
        if (!writeStarted) {
            Log.e(TAG, "Could not start CCCD write for ${characteristic.uuid}")
            notificationWriteInFlight = false
            writeNextNotification(g)
        }
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
        Log.d(TAG, "sendCommand: writing 0x%04X to CMD characteristic".format(commandId))
        writeToCharacteristic(char, commandId16Bytes(commandId))
    }

    fun disconnect() {
        mainHandler.removeCallbacks(connectTimeoutRunnable)
        stopScan()
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        cmdChar = null
        libraryCmdChar = null
        playlistCmdChar = null
        clearNotificationQueue()
        _connectionState.value = ConnectionState.Idle
        _status.value = null
        _selectedViewId.value = null
        _libraryListing.value = emptyList()
        _playlistNameEntries.value = null
        _playlistOpResult.value = null
        playlistNameMode = false
    }

    fun setSelectedViewId(id: Int?) {
        _selectedViewId.value = id
    }

    private fun writeLibraryCommand(commandId: Int, param: Int) {
        playlistNameMode = false
        val char = libraryCmdChar ?: run { Log.w(TAG, "library cmd 0x%04X: libraryCmdChar is null".format(commandId)); return }
        val bytes = commandId16Bytes(commandId) + byteArrayOf(
            (param and 0xFF).toByte(),
            ((param shr 8) and 0xFF).toByte()
        )
        writeToCharacteristic(char, bytes)
    }

    fun browseRoot() = writeLibraryCommand(BleProtocol.CMD_BROWSE_REQUEST, BleProtocol.LIB_BROWSE_ROOT)
    fun browseUp() = writeLibraryCommand(BleProtocol.CMD_BROWSE_REQUEST, BleProtocol.LIB_BROWSE_UP)
    fun browseInto(index: Int) = writeLibraryCommand(BleProtocol.CMD_BROWSE_REQUEST, index)
    fun addTrack(index: Int) = writeLibraryCommand(BleProtocol.CMD_ADD_TRACK, index)
    fun addFolder(index: Int) = writeLibraryCommand(BleProtocol.CMD_ADD_FOLDER, index)
    fun replaceWithFolder(index: Int) = writeLibraryCommand(BleProtocol.CMD_REPLACE_WITH_FOLDER, index)
    fun playTrack(index: Int) = writeLibraryCommand(BleProtocol.CMD_PLAY_TRACK, index)
    fun removeTrack(index: Int) = writeLibraryCommand(BleProtocol.CMD_REMOVE_TRACK, index)
    fun requestPlaylist() {
        _libraryListing.value = emptyList()
        writeLibraryCommand(BleProtocol.CMD_PLAYLIST_REQUEST, 0)
    }

    fun requestPlaylistNames() {
        playlistNameMode = true
        _playlistNameEntries.value = null
        writeLibraryCommand(BleProtocol.CMD_PLAYLIST_LIST_REQUEST, 0)
    }

    fun clearPlaylistOpResult() {
        _playlistOpResult.value = null
    }

    private fun writePlaylistCommand(commandId: Int, name: String) {
        val char = playlistCmdChar ?: run { Log.w(TAG, "playlist cmd 0x%04X: playlistCmdChar is null".format(commandId)); return }
        val fullNameBytes = name.toByteArray(Charsets.UTF_8)
        val nameBytes = fullNameBytes.copyOf(minOf(fullNameBytes.size, 55))
        writeToCharacteristic(char, commandId16Bytes(commandId) + nameBytes)
    }

    fun savePlaylist(name: String) = writePlaylistCommand(BleProtocol.CMD_PLAYLIST_SAVE, name)
    fun overwritePlaylist(name: String) = writePlaylistCommand(BleProtocol.CMD_PLAYLIST_SAVE_OVERWRITE, name)
    fun loadPlaylist(name: String) = writePlaylistCommand(BleProtocol.CMD_PLAYLIST_LOAD, name)
    fun deletePlaylist(name: String) = writePlaylistCommand(BleProtocol.CMD_PLAYLIST_DELETE, name)

    companion object {
        private const val TAG = "BoardBleManager"
    }
}

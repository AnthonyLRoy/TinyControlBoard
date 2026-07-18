package com.tinycb.remote.ui

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.ViewGroup
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.ble.ConnectionState
import com.tinycb.remote.databinding.ActivityScanBinding
import com.tinycb.remote.databinding.ItemBleDeviceBinding
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

@SuppressLint("MissingPermission")
class ScanActivity : AppCompatActivity() {

    private lateinit var b: ActivityScanBinding
    private val vm: MainViewModel by viewModels()

    private val deviceAdapter = DeviceAdapter { device -> vm.bleManager.connect(device) }

    // ── Permission launcher ──────────────────────────────────────────────
    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { results ->
        if (results.values.all { it }) {
            startScan()
        } else {
            Toast.makeText(this, "Bluetooth permissions required", Toast.LENGTH_LONG).show()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityScanBinding.inflate(layoutInflater)
        setContentView(b.root)

        b.rvDevices.layoutManager = LinearLayoutManager(this)
        b.rvDevices.adapter = deviceAdapter

        b.btnScan.setOnClickListener { requestPermissionsAndScan() }

        lifecycleScope.launch {
            vm.connectionState.collectLatest { state ->
                when (state) {
                    is ConnectionState.Idle -> {
                        b.progressScan.visibility = android.view.View.GONE
                        b.btnScan.isEnabled = true
                        b.tvScanSubtitle.text = "Tap Scan to search for TinyControlBoard"
                    }
                    is ConnectionState.Scanning -> {
                        b.progressScan.visibility = android.view.View.VISIBLE
                        b.btnScan.isEnabled = false
                        b.tvScanSubtitle.text = getString(com.tinycb.remote.R.string.scan_subtitle)
                    }
                    is ConnectionState.DevicesFound -> {
                        b.progressScan.visibility = android.view.View.VISIBLE
                        deviceAdapter.submitList(state.devices)
                    }
                    is ConnectionState.Connecting -> {
                        b.progressScan.visibility = android.view.View.VISIBLE
                        b.tvScanSubtitle.text = "Connecting…"
                    }
                    is ConnectionState.Connected -> {
                        b.progressScan.visibility = android.view.View.GONE
                        val intent = Intent(this@ScanActivity, MainActivity::class.java)
                        startActivity(intent)
                        // Do not finish — back from MainActivity returns here
                    }
                    is ConnectionState.Disconnected -> {
                        b.progressScan.visibility = android.view.View.GONE
                        b.btnScan.isEnabled = true
                        b.tvScanSubtitle.text = "Disconnected. Tap Scan to reconnect."
                        deviceAdapter.submitList(emptyList())
                    }
                    is ConnectionState.Error -> {
                        b.progressScan.visibility = android.view.View.GONE
                        b.btnScan.isEnabled = true
                        Toast.makeText(this@ScanActivity, state.message, Toast.LENGTH_LONG).show()
                    }
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        // Stop any lingering scan when returning from MainActivity
        val state = vm.connectionState.value
        if (state is ConnectionState.Connected || state is ConnectionState.Scanning) {
            vm.bleManager.stopScan()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        vm.bleManager.stopScan()
    }

    private fun requestPermissionsAndScan() {
        val needed = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            arrayOf(
                android.Manifest.permission.BLUETOOTH_SCAN,
                android.Manifest.permission.BLUETOOTH_CONNECT
            )
        } else {
            arrayOf(
                android.Manifest.permission.BLUETOOTH,
                android.Manifest.permission.BLUETOOTH_ADMIN,
                android.Manifest.permission.ACCESS_FINE_LOCATION
            )
        }

        val allGranted = needed.all {
            checkSelfPermission(it) == android.content.pm.PackageManager.PERMISSION_GRANTED
        }

        if (allGranted) startScan() else permissionLauncher.launch(needed)
    }

    private fun startScan() {
        val btAdapter = BluetoothAdapter.getDefaultAdapter()
        if (btAdapter == null || !btAdapter.isEnabled) {
            Toast.makeText(this, "Please enable Bluetooth", Toast.LENGTH_SHORT).show()
            return
        }
        deviceAdapter.submitList(emptyList())
        vm.bleManager.startScan()
    }

    // ── Device list adapter ──────────────────────────────────────────────────
    private class DeviceAdapter(
        private val onConnect: (BluetoothDevice) -> Unit
    ) : ListAdapter<BluetoothDevice, DeviceAdapter.VH>(DEVICE_DIFF) {

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH =
            VH(ItemBleDeviceBinding.inflate(LayoutInflater.from(parent.context), parent, false))

        override fun onBindViewHolder(holder: VH, position: Int) =
            holder.bind(getItem(position), onConnect)

        @SuppressLint("MissingPermission")
        class VH(private val b: ItemBleDeviceBinding) : RecyclerView.ViewHolder(b.root) {
            fun bind(device: BluetoothDevice, onConnect: (BluetoothDevice) -> Unit) {
                b.tvDeviceName.text = device.name ?: "Unknown"
                b.tvDeviceAddress.text = device.address
                b.root.setOnClickListener { onConnect(device) }
            }
        }

        companion object {
            val DEVICE_DIFF = object : DiffUtil.ItemCallback<BluetoothDevice>() {
                override fun areItemsTheSame(a: BluetoothDevice, b: BluetoothDevice) =
                    a.address == b.address
                override fun areContentsTheSame(a: BluetoothDevice, b: BluetoothDevice) =
                    a.address == b.address
            }
        }
    }
}

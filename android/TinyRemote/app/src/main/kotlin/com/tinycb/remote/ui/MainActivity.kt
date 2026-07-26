package com.tinycb.remote.ui

import android.content.res.ColorStateList
import android.os.Bundle
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.GridLayoutManager
import com.tinycb.remote.ble.ConnectionState
import com.tinycb.remote.databinding.ActivityMainBinding
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {

    companion object {
        /** commandId for Power (matches ButtonCatalog's former Power entry). */
        private const val POWER_COMMAND_ID = 0x0001
    }

    private lateinit var b: ActivityMainBinding
    private val vm: MainViewModel by viewModels()
    private lateinit var buttonAdapter: ButtonPanelAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityMainBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        buttonAdapter = ButtonPanelAdapter { btn ->
            vm.sendCommand(btn.commandId)
        }
        buttonAdapter.submitList(vm.buttons)

        // Power is deliberately NOT in the button grid — it's a small icon in the top status
        // strip so it can't be pressed by accident alongside the frequently-used buttons.
        b.btnPower.setOnClickListener {
            vm.sendCommand(POWER_COMMAND_ID)
        }

        b.rvButtons.apply {
            layoutManager = GridLayoutManager(this@MainActivity, 4).apply {
                spanSizeLookup = object : GridLayoutManager.SpanSizeLookup() {
                    override fun getSpanSize(position: Int): Int =
                        buttonAdapter.currentList.getOrNull(position)?.spanSize ?: 1
                }
            }
            adapter = buttonAdapter
            setHasFixedSize(true)
        }

        // Observe board status → update power state chip + LED dots
        lifecycleScope.launch {
            vm.boardStatus.collectLatest { status ->
                val style = PowerStateUi.styleFor(status?.powerStateName)
                b.tvStateChip.text = style.label
                b.tvStateChip.backgroundTintList =
                    ColorStateList.valueOf(ContextCompat.getColor(this@MainActivity, style.colorRes))
                buttonAdapter.updateStatus(status)
            }
        }

        // Reflect connection state: show connected device name in toolbar subtitle;
        // if BLE disconnects, go back to scan screen
        lifecycleScope.launch {
            vm.connectionState.collectLatest { state ->
                if (state is ConnectionState.Connected) {
                    supportActionBar?.subtitle = state.deviceName?.let { "Connected • $it" } ?: "Connected"
                }
                if (state is ConnectionState.Disconnected || state is ConnectionState.Error) {
                    finish()
                }
            }
        }
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            finish()
            return true
        }
        return super.onOptionsItemSelected(item)
    }
}

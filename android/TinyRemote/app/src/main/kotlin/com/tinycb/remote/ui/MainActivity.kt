package com.tinycb.remote.ui

import android.content.res.ColorStateList
import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.GridLayoutManager
import com.tinycb.remote.R
import com.tinycb.remote.ble.ConnectionState
import com.tinycb.remote.databinding.ActivityMainBinding
import com.tinycb.remote.model.GridItem
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

    private val progressHandler = Handler(Looper.getMainLooper())
    private var trackDurationSec = 0
    private var trackElapsedSec = 0
    private var isTrackPlaying = false
    private var trackProgressUpdatedAtMs = 0L
    private val progressTicker = object : Runnable {
        override fun run() {
            updateProgressBar()
            progressHandler.postDelayed(this, 500L)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityMainBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        buttonAdapter = ButtonPanelAdapter { commandId ->
            when (commandId) {
                0x0107 -> startActivity(Intent(this@MainActivity, ViewSelectionActivity::class.java))
                0x0114 -> vm.toggleDisplay()
                else   -> vm.sendCommand(commandId)
            }
        }
        
        lifecycleScope.launch {
            vm.buttons.collectLatest { buttons ->
                buttonAdapter.submitList(buttons)
            }
        }

        // Power is deliberately NOT in the button grid — it's a small icon in the top status
        // strip so it can't be pressed by accident alongside the frequently-used buttons.
        b.btnPower.setOnClickListener {
            vm.sendCommand(POWER_COMMAND_ID)
        }

        b.rvButtons.apply {
            layoutManager = GridLayoutManager(this@MainActivity, 4).apply {
                spanSizeLookup = object : GridLayoutManager.SpanSizeLookup() {
                    override fun getSpanSize(position: Int): Int =
                        (buttonAdapter.currentList.getOrNull(position) as? GridItem)?.spanSize ?: 1
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
                
                // Update power button tint based on state
                val iconColorRes = if (status?.powerStateName == "ON") R.color.state_on else R.color.btn_bg_power
                b.btnPower.imageTintList = ColorStateList.valueOf(ContextCompat.getColor(this@MainActivity, iconColorRes))

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

        lifecycleScope.launch {
            vm.nowPlaying.collectLatest { track ->
                if (track.isNullOrEmpty()) {
                    b.tvNowPlaying.visibility = android.view.View.GONE
                } else {
                    b.tvNowPlaying.text = track
                    b.tvNowPlaying.visibility = android.view.View.VISIBLE
                    b.tvNowPlaying.isSelected = true  // required for marquee scroll
                }
            }
        }

        lifecycleScope.launch {
            vm.boardStatus.collectLatest { status ->
                trackDurationSec = status?.trackDurationSec ?: 0
                trackElapsedSec = status?.trackElapsedSec ?: 0
                isTrackPlaying = status?.isTrackPlaying ?: false
                trackProgressUpdatedAtMs = status?.trackProgressUpdatedAtMs ?: 0L
                b.layoutTrackProgress.visibility =
                    if (trackDurationSec > 0) android.view.View.VISIBLE else android.view.View.GONE
                updateProgressBar()
            }
        }
    }

    /** Interpolates elapsed time between BLE updates (which arrive every ~2s) for a smooth bar. */
    private fun updateProgressBar() {
        if (trackDurationSec <= 0) return
        val driftSec = if (isTrackPlaying && trackProgressUpdatedAtMs > 0)
            (System.currentTimeMillis() - trackProgressUpdatedAtMs) / 1000 else 0L
        val interpolatedElapsed = (trackElapsedSec + driftSec).coerceIn(0L, trackDurationSec.toLong())
        val remaining = trackDurationSec - interpolatedElapsed
        b.progressTrack.progress = ((interpolatedElapsed * 1000) / trackDurationSec).toInt()
        b.tvElapsed.text = formatSeconds(interpolatedElapsed)
        b.tvRemaining.text = "-" + formatSeconds(remaining)
    }

    private fun formatSeconds(totalSeconds: Long): String {
        val m = totalSeconds / 60
        val s = totalSeconds % 60
        return "%d:%02d".format(m, s)
    }

    override fun onResume() {
        super.onResume()
        progressHandler.post(progressTicker)
    }

    override fun onPause() {
        super.onPause()
        progressHandler.removeCallbacks(progressTicker)
    }

    override fun onDestroy() {
        super.onDestroy()
        if (isFinishing) {
            vm.bleManager.disconnect()
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

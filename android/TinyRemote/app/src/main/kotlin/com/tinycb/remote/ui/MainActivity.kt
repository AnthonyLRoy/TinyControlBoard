package com.tinycb.remote.ui

import android.animation.ValueAnimator
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
import com.tinycb.remote.data.ButtonCatalog
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

    private var powerButtonAnimator: ValueAnimator? = null
    private var isManuallyFlashing = false
    private var lastKnownPowerState: String? = null
    private var powerStateAtClick: String? = null
    private val manualFlashTimeout = Runnable {
        if (isManuallyFlashing) {
            android.util.Log.e("MainActivity", "Manual flash timed out, resetting")
            isManuallyFlashing = false
            updatePowerUi(lastKnownPowerState)
        }
    }

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
        android.util.Log.e("MainActivity", "onCreate started")
        b = ActivityMainBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.setDisplayShowTitleEnabled(false)

        buttonAdapter = ButtonPanelAdapter { commandId ->
            when (commandId) {
                ButtonCatalog.CMD_VIEW_MENU   -> startActivity(Intent(this@MainActivity, ViewSelectionActivity::class.java))
                ButtonCatalog.CMD_LIBRARY     -> startActivity(Intent(this@MainActivity, LibraryActivity::class.java))
                ButtonCatalog.CMD_DISPLAY_OFF -> vm.toggleDisplay()
                else                          -> vm.sendCommand(commandId)
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
            android.util.Log.e("MainActivity", "Power button clicked. Current known state: $lastKnownPowerState")
            isManuallyFlashing = true
            powerStateAtClick = lastKnownPowerState
            updatePowerUi(lastKnownPowerState) // Update UI first for instant response
            vm.sendCommand(POWER_COMMAND_ID)
            
            // Auto-reset manual flash after 4 seconds if no update received
            progressHandler.removeCallbacks(manualFlashTimeout)
            progressHandler.postDelayed(manualFlashTimeout, 4000L)
        }

        b.tvNowPlaying.setOnClickListener {
            startActivity(Intent(this@MainActivity, PlaylistActivity::class.java))
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

        // Observe board status → update LED dots
        lifecycleScope.launch {
            vm.boardStatus.collect { status ->
                val stateName = status?.powerStateName
                android.util.Log.e("MainActivity", "Observed powerStateName: '$stateName'")
                
                // If we were manually flashing and we get a confirmed stable state change, stop manual mode
                if (isManuallyFlashing && stateName != null && !isPowerStateTransitioning(stateName)) {
                    if (stateName != powerStateAtClick) {
                        android.util.Log.e("MainActivity", "State change confirmed ($powerStateAtClick -> $stateName), stopping manual flash")
                        isManuallyFlashing = false
                        progressHandler.removeCallbacks(manualFlashTimeout)
                        powerStateAtClick = null
                    }
                }
                
                lastKnownPowerState = stateName
                updatePowerUi(stateName)
                refreshStatusBadge()
                buttonAdapter.updateStatus(status)
            }
        }

        // Reflect connection state: show connected status in toolbar;
        // if BLE disconnects, go back to scan screen
        lifecycleScope.launch {
            vm.connectionState.collectLatest { state ->
                refreshStatusBadge()

                if (state is ConnectionState.Disconnected || state is ConnectionState.Error) {
                    finish()
                }
            }
        }

        lifecycleScope.launch {
            vm.nowPlaying.collectLatest { track ->
                b.tvNowPlaying.text = if (track.isNullOrEmpty()) {
                    getString(R.string.nothing_playing)
                } else {
                    track
                }
                b.tvNowPlaying.visibility = android.view.View.VISIBLE
                b.tvNowPlaying.isSelected = true  // required for marquee scroll
            }
        }

        lifecycleScope.launch {
            vm.boardStatus.collectLatest { status ->
                trackDurationSec = status?.trackDurationSec ?: 0
                trackElapsedSec = status?.trackElapsedSec ?: 0
                isTrackPlaying = status?.isTrackPlaying ?: false
                trackProgressUpdatedAtMs = status?.trackProgressUpdatedAtMs ?: 0L
                b.layoutTrackProgress.visibility = android.view.View.VISIBLE
                updateProgressBar()
            }
        }
    }

    /** Interpolates elapsed time between BLE updates (which arrive every ~2s) for a smooth bar. */
    private fun updateProgressBar() {
        if (trackDurationSec <= 0) {
            b.progressTrack.progress = 0
            b.tvElapsed.text = formatSeconds(0)
            b.tvRemaining.text = "-0:00"
            return
        }
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
        stopPowerFlashing()
        if (isFinishing) {
            vm.bleManager.disconnect()
        }
    }

    private fun isPowerStateTransitioning(state: String?): Boolean {
        if (state == null || state == "UNKNOWN") return false
        val stableStates = listOf("ON", "OFF", "SLEEP", "DEEP SLEEP")
        return state !in stableStates
    }

    private fun updatePowerUi(stateName: String?) {
        val isTransitioning = isPowerStateTransitioning(stateName) || isManuallyFlashing
        
        val iconColorRes = when {
            isTransitioning -> R.color.state_busy
            stateName == "ON" -> R.color.state_on
            else -> R.color.btn_bg_power
        }
        
        val colorInt = ContextCompat.getColor(this@MainActivity, iconColorRes)
        android.util.Log.e("MainActivity", "Updating Power Button: state=$stateName, transitioning=$isTransitioning, colorRes=$iconColorRes, manual=$isManuallyFlashing")
        
        b.btnPower.setColorFilter(colorInt)

        if (isTransitioning) {
            startPowerFlashing()
        } else {
            stopPowerFlashing()
        }
    }

    private fun refreshStatusBadge() {
        val connState = vm.connectionState.value
        val powerState = lastKnownPowerState

        val (text, colorRes) = when (connState) {
            is ConnectionState.Connected -> {
                val deviceName = connState.deviceName ?: getString(R.string.connected)
                val baseText = getString(R.string.connected_with_device, deviceName)
                val powerSuffix = if (powerState != null) " (${powerState.replace('_', ' ')})" else ""
                
                val bgColor = when (powerState) {
                    "SLEEP", "DEEP_SLEEP" -> R.color.status_connected_sleep
                    "ON" -> R.color.status_connected_on
                    else -> R.color.status_connected_on
                }
                (baseText + powerSuffix) to bgColor
            }
            is ConnectionState.Connecting -> {
                getString(R.string.connecting) to R.color.status_disconnected
            }
            else -> {
                getString(R.string.disconnected) to R.color.status_disconnected
            }
        }

        b.tvConnectionStatus.text = text
        b.tvConnectionStatus.backgroundTintList = ColorStateList.valueOf(
            ContextCompat.getColor(this, colorRes)
        )
    }

    private fun startPowerFlashing() {
        if (powerButtonAnimator?.isRunning == true) return
        android.util.Log.e("MainActivity", "Starting power flashing animation")
        powerButtonAnimator = ValueAnimator.ofFloat(1.0f, 0.05f).apply {
            duration = 300
            repeatMode = ValueAnimator.REVERSE
            repeatCount = ValueAnimator.INFINITE
            addUpdateListener { animator ->
                b.btnPower.alpha = animator.animatedValue as Float
            }
            start()
        }
    }

    private fun stopPowerFlashing() {
        if (powerButtonAnimator == null) return
        android.util.Log.i("MainActivity", "Stopping power flashing animation")
        powerButtonAnimator?.cancel()
        powerButtonAnimator = null
        b.btnPower.alpha = 1.0f
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            finish()
            return true
        }
        return super.onOptionsItemSelected(item)
    }
}

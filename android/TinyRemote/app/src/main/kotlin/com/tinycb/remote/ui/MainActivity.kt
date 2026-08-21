package com.tinycb.remote.ui

import android.animation.ValueAnimator
import android.content.res.ColorStateList
import android.content.Intent
import android.os.Bundle
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
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {

    private lateinit var b: ActivityMainBinding
    private val vm: MainViewModel by viewModels()
    private lateinit var buttonAdapter: ButtonPanelAdapter

    private var powerButtonAnimator: ValueAnimator? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityMainBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.setDisplayShowTitleEnabled(false)

        buttonAdapter = ButtonPanelAdapter { commandId ->
            when (commandId) {
                ButtonCatalog.CMD_VIEW_MENU   -> startActivity(Intent(this, ViewSelectionActivity::class.java))
                ButtonCatalog.CMD_LIBRARY     -> startActivity(Intent(this, LibraryActivity::class.java))
                ButtonCatalog.CMD_DISPLAY_OFF -> vm.toggleDisplay()
                else                          -> vm.sendCommand(commandId)
            }
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

        b.progressTrack.max = 10000
        b.btnPower.setOnClickListener { vm.onPowerClicked() }
        b.tvNowPlaying.setOnClickListener {
            startActivity(Intent(this, PlaylistActivity::class.java))
        }

        observeViewModel()
    }

    private fun observeViewModel() {
        lifecycleScope.launch {
            vm.buttons.collectLatest { buttonAdapter.submitList(it) }
        }

        lifecycleScope.launch {
            vm.connectionState.collectLatest { state ->
                refreshStatusBadge(state, vm.powerState.value)
                if (state is ConnectionState.Disconnected || state is ConnectionState.Error) {
                    finish()
                }
            }
        }

        lifecycleScope.launch {
            vm.nowPlaying.collectLatest { track ->
                b.tvNowPlaying.text = track.takeUnless { it.isNullOrEmpty() } ?: getString(R.string.nothing_playing)
                b.tvNowPlaying.isSelected = true
            }
        }

        lifecycleScope.launch {
            vm.trackProgress.collectLatest { progress ->
                b.layoutTrackProgress.visibility = if (progress.isVisible) android.view.View.VISIBLE else android.view.View.GONE
                updateProgressBar(progress)
            }
        }

        lifecycleScope.launch {
            vm.powerState.combine(vm.isPowerFlashing) { stateName, flashing ->
                stateName to flashing
            }.collectLatest { (stateName, flashing) ->
                updatePowerUi(stateName, flashing)
            }
        }

        lifecycleScope.launch {
            vm.boardStatus.collectLatest { status ->
                buttonAdapter.updateStatus(status)
                refreshStatusBadge(vm.connectionState.value, status?.powerStateName)
            }
        }
    }

    private fun updateProgressBar(progress: MainViewModel.ProgressState) {
        if (progress.duration <= 0) {
            b.progressTrack.progress = 0
            b.tvElapsed.text = formatSeconds(0)
            b.tvRemaining.text = "-0:00"
            return
        }

        val driftSec = if (progress.lastUpdateMs > 0)
            (System.currentTimeMillis() - progress.lastUpdateMs) / 1000.0 else 0.0
        val interpolatedElapsed = (progress.elapsed + driftSec).coerceIn(0.0, progress.duration.toDouble())
        val remaining = progress.duration - interpolatedElapsed

        // Use higher resolution (max=10000) for smoother movement
        b.progressTrack.progress = ((interpolatedElapsed * 10000) / progress.duration).toInt()
        b.tvElapsed.text = formatSeconds(interpolatedElapsed.toLong())
        b.tvRemaining.text = "-" + formatSeconds(remaining.toLong())
    }

    private fun formatSeconds(totalSeconds: Long): String {
        return "%d:%02d".format(totalSeconds / 60, totalSeconds % 60)
    }

    private fun updatePowerUi(stateName: String?, isFlashing: Boolean) {
        val iconColorRes = when {
            isFlashing -> R.color.state_busy
            stateName == "ON" -> R.color.state_on
            else -> R.color.btn_bg_power
        }
        
        b.btnPower.setColorFilter(ContextCompat.getColor(this, iconColorRes))

        if (isFlashing) {
            startPowerFlashing()
        } else {
            stopPowerFlashing()
        }
    }

    private fun refreshStatusBadge(connState: ConnectionState, powerState: String?) {
        val (text, colorRes) = when (connState) {
            is ConnectionState.Connected -> {
                val baseText = getString(R.string.connected)
                val powerSuffix = if (powerState != null) " (${powerState.replace('_', ' ')})" else ""
                val bgColor = when (powerState) {
                    "SLEEP", "DEEP SLEEP" -> R.color.status_connected_sleep
                    "ON" -> R.color.status_connected_on
                    else -> R.color.status_connected_on
                }
                (baseText + powerSuffix) to bgColor
            }
            is ConnectionState.Connecting -> getString(R.string.connecting) to R.color.status_disconnected
            else -> getString(R.string.disconnected) to R.color.status_disconnected
        }

        b.tvConnectionStatus.text = text
        b.tvConnectionStatus.backgroundTintList = ColorStateList.valueOf(ContextCompat.getColor(this, colorRes))
    }

    private fun startPowerFlashing() {
        if (powerButtonAnimator?.isRunning == true) return
        powerButtonAnimator = ValueAnimator.ofFloat(1.0f, 0.05f).apply {
            duration = 300
            repeatMode = ValueAnimator.REVERSE
            repeatCount = ValueAnimator.INFINITE
            addUpdateListener { b.btnPower.alpha = it.animatedValue as Float }
            start()
        }
    }

    private fun stopPowerFlashing() {
        powerButtonAnimator?.cancel()
        powerButtonAnimator = null
        b.btnPower.alpha = 1.0f
    }

    override fun onDestroy() {
        super.onDestroy()
        stopPowerFlashing()
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

package com.tinycb.remote.ui

import android.animation.ValueAnimator
import android.content.res.ColorStateList
import android.content.Intent
import android.os.Bundle
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.compose.ui.platform.ComposeView
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

        b.composeBackground.setContent {
            BrushedAluminumSurface(shade = MetallicShade.Titanium)
        }

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.setDisplayShowTitleEnabled(false)

        buttonAdapter = ButtonPanelAdapter { commandId ->
            when (commandId) {
                ButtonCatalog.CMD_LIBRARY     -> startActivity(Intent(this, LibraryActivity::class.java))
                else                          -> vm.sendCommand(commandId)
            }
        }

        b.btnPrev.setOnClickListener { vm.sendCommand(ButtonCatalog.CMD_PREV) }
        b.btnPlayPause.setOnClickListener { vm.sendCommand(ButtonCatalog.CMD_PLAY) }
        b.btnNext.setOnClickListener { vm.sendCommand(ButtonCatalog.CMD_NEXT) }
        b.btnMenu.setOnClickListener { startActivity(Intent(this, ViewSelectionActivity::class.java)) }
        b.btnShuffle.setOnClickListener { vm.sendCommand(ButtonCatalog.CMD_SHUFFLE) }
        b.btnRepeat.setOnClickListener { vm.sendCommand(ButtonCatalog.CMD_REPEAT) }
        b.btnBrightnessDown.setOnClickListener { vm.sendCommand(ButtonCatalog.CMD_BRIGHT_DOWN) }
        b.btnBrightnessUp.setOnClickListener { vm.sendCommand(ButtonCatalog.CMD_BRIGHT_UP) }
        b.btnDisplayToggle.setOnClickListener { vm.toggleDisplay() }

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
        b.progressTrack.setOnTouchListener { view, event ->
            when (event.action) {
                android.view.MotionEvent.ACTION_DOWN,
                android.view.MotionEvent.ACTION_MOVE -> {
                    val percent = ((event.x / view.width) * 100).toInt().coerceIn(0, 100)
                    vm.seekToPercent(percent)
                    true
                }
                else -> false
            }
        }
        b.btnPower.setOnClickListener { onPowerButtonPressed() }
        b.tvNowPlaying.setOnClickListener {
            startActivity(Intent(this, PlaylistActivity::class.java))
        }

        observeViewModel()
    }

    private fun onPowerButtonPressed() {
        if (vm.isPoweredOn()) {
            showPowerOptionsDialog()
        } else {
            vm.onPowerClicked()
        }
    }

    private fun showPowerOptionsDialog() {
        androidx.appcompat.app.AlertDialog.Builder(this)
            .setTitle(R.string.power_options_title)
            .setItems(arrayOf(getString(R.string.power_sleep), getString(R.string.power_deep_sleep))) { _, which ->
                when (which) {
                    0 -> vm.onPowerClicked()
                    1 -> vm.onDeepSleepClicked()
                }
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
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
            vm.albumArt.collectLatest { art ->
                if (art != null) {
                    b.ivAlbumArt.scaleType = android.widget.ImageView.ScaleType.CENTER_CROP
                    b.ivAlbumArt.imageTintList = null
                    b.ivAlbumArt.setImageBitmap(art)
                } else {
                    b.ivAlbumArt.scaleType = android.widget.ImageView.ScaleType.CENTER_INSIDE
                    b.ivAlbumArt.imageTintList = ColorStateList.valueOf(ContextCompat.getColor(this@MainActivity, R.color.menu_icon_default))
                    b.ivAlbumArt.setImageResource(R.drawable.ic_album)
                }
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
                updateTransportUi(status)
                refreshStatusBadge(vm.connectionState.value, status?.powerStateName)
            }
        }
    }

    /** Mirrors the LED bitmask onto the fixed transport row's highlight/tint state, since
     *  these buttons are no longer part of the [ButtonPanelAdapter] grid. */
    private fun updateTransportUi(status: com.tinycb.remote.model.BoardStatus?) {
        val accent = ContextCompat.getColor(this, R.color.menu_accent)
        val default = ContextCompat.getColor(this, R.color.menu_icon_default)

        val isPlaying = status?.isTrackPlaying == true
        b.ivPlayPauseIcon.setImageResource(if (isPlaying) R.drawable.ic_play_pause else R.drawable.ic_play)
        b.playPauseHighlight.visibility = if (isPlaying) android.view.View.VISIBLE else android.view.View.GONE

        val bitmask = status?.buttonLedBitmask ?: 0
        val isShuffleOn = (bitmask and (1 shl 9)) != 0
        b.shuffleHighlight.visibility = if (isShuffleOn) android.view.View.VISIBLE else android.view.View.GONE
        b.ivShuffleIcon.imageTintList = ColorStateList.valueOf(if (isShuffleOn) accent else default)

        val isRepeatOn = (bitmask and (1 shl 8)) != 0
        b.repeatHighlight.visibility = if (isRepeatOn) android.view.View.VISIBLE else android.view.View.GONE
        b.ivRepeatIcon.imageTintList = ColorStateList.valueOf(if (isRepeatOn) accent else default)

        // Display LED reports the opposite of the desired highlight (see BrightnessAction);
        // the toggle should look "active" when the monitor is ON.
        val isDisplayOn = (bitmask and (1 shl 6)) == 0
        b.btnDisplayToggle.imageTintList = ColorStateList.valueOf(if (isDisplayOn) accent else default)
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
                    "GOING TO SLEEP", "GOING INTO DEEP SLEEP" -> R.color.status_connected_going_to_sleep
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

package com.tinycb.remote.ui

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.tinycb.remote.ble.BoardBleManagerHolder
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.launch

/** Returns to the power controls if the board becomes unavailable while browsing. */
abstract class ConnectedControlActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val manager = BoardBleManagerHolder.get(application)
        lifecycleScope.launch {
            combine(manager.connectionState, manager.status, ::controlsAvailable).collect { available ->
                if (!available) finish()
            }
        }
    }
}

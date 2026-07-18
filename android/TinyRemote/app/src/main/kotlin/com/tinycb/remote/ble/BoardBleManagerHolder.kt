package com.tinycb.remote.ble

import android.content.Context

/**
 * Application-wide singleton holder for [BoardBleManager].
 *
 * ScanActivity and MainActivity each have their own Activity-scoped ViewModel
 * (via `by viewModels()`), which means each would otherwise get a *different*
 * BoardBleManager instance — the one that connects (in ScanActivity) is not
 * the one that sends commands (in MainActivity). This holder ensures every
 * ViewModel talks to the exact same BLE connection.
 */
object BoardBleManagerHolder {
    @Volatile private var instance: BoardBleManager? = null

    fun get(context: Context): BoardBleManager =
        instance ?: synchronized(this) {
            instance ?: BoardBleManager(context.applicationContext).also { instance = it }
        }
}

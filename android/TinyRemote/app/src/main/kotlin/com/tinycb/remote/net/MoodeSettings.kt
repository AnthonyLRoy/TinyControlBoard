package com.tinycb.remote.net

import android.content.Context

/** Persists the user-configured moOde/board LAN IP address (or hostname) across app launches. */
object MoodeSettings {

    private const val PREFS_NAME = "moode_settings"
    private const val KEY_HOST = "moode_host"

    fun getHost(context: Context): String =
        context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getString(KEY_HOST, CoverArtFetcher.DEFAULT_BOARD_HOST)
            ?: CoverArtFetcher.DEFAULT_BOARD_HOST

    fun setHost(context: Context, host: String) {
        context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putString(KEY_HOST, host.trim())
            .apply()
    }
}

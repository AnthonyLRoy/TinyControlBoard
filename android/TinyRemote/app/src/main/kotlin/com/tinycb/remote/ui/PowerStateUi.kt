package com.tinycb.remote.ui

import androidx.annotation.ColorRes
import com.tinycb.remote.R

/**
 * Maps the firmware's power-state name (SystemState::PowerState ordinal name, e.g. "ON",
 * "OFF", "SLEEP", "TURNING_ON", "SHUTTING_DOWN") to a chip label + color for the header strip.
 */
object PowerStateUi {

    data class Style(val label: String, @ColorRes val colorRes: Int)

    fun styleFor(powerStateName: String?): Style = when (powerStateName) {
        "ON" -> Style("\u25CF ON", R.color.state_on)
        "OFF" -> Style("\u25CF OFF", R.color.state_off)
        "SLEEP" -> Style("\u25CF SLEEP", R.color.state_sleep)
        null -> Style("\u25CF \u2013", R.color.state_busy)
        else -> Style("\u25CF ${powerStateName.replace('_', ' ')}", R.color.state_busy)
    }
}

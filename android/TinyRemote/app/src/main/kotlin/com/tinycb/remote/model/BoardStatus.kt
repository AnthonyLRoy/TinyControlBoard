package com.tinycb.remote.model

data class BoardStatus(
    val powerStateName: String,
    val buttonLedBitmask: Int,
    val nowPlaying: String? = null,
    val trackElapsedSec: Int = 0,
    val trackDurationSec: Int = 0,
    val isTrackPlaying: Boolean = false,
    val trackProgressUpdatedAtMs: Long = 0L
)

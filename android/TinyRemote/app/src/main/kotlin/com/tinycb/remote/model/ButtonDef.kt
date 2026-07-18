package com.tinycb.remote.model

data class ButtonDef(
    val index: Int,
    val name: String,
    val commandId: Int,
    /** Bit index in buttonLedBitmask, or -1 if this button has no LED. */
    val bitmaskBit: Int,
    val iconRes: Int
)

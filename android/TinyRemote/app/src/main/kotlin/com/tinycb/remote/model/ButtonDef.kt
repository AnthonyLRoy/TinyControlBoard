package com.tinycb.remote.model

import com.tinycb.remote.R

data class ButtonDef(
    val index: Int,
    val name: String,
    val commandId: Int,
    /** Bit index in buttonLedBitmask, or -1 if this button has no LED. */
    val bitmaskBit: Int,
    val iconRes: Int,
    /** Number of grid columns this button spans (out of 4). */
    val spanSize: Int = 1,
    /** Card background color resource for this button. */
    val backgroundColorRes: Int = R.color.btn_bg_default
)

package com.tinycb.remote.data

import com.tinycb.remote.R
import com.tinycb.remote.model.ButtonDef

/**
 * Hardcoded catalog of all 16 physical buttons on the TinyControlBoard.
 * commandId values match the CommandId enum in lib/protocol/uartProtocol.hpp.
 * bitmaskBit is the bit index in the 16-bit buttonLedBitmask (-1 = no LED).
 */
object ButtonCatalog {
    val buttons: List<ButtonDef> = listOf(
        // Row 1: skip back, prev, next, skip
        ButtonDef(4,  "◀◀ Skip",   0x0105,  4, R.drawable.ic_fast_rewind),
        ButtonDef(1,  "Prev",       0x0101,  1, R.drawable.ic_skip_previous),
        ButtonDef(2,  "Next",       0x0100,  2, R.drawable.ic_skip_next),
        ButtonDef(3,  "Skip ▶▶",   0x0104,  3, R.drawable.ic_fast_forward),
        // Row 2: stop, play/pause, repeat, shuffle
        ButtonDef(6,  "Stop",       0x0103,  6, R.drawable.ic_stop),
        ButtonDef(5,  "Play/Pause", 0x0102,  5, R.drawable.ic_play_pause),
        ButtonDef(8,  "Repeat",     0x011C,  8, R.drawable.ic_repeat),
        ButtonDef(9,  "Shuffle",    0x011F,  9, R.drawable.ic_shuffle),
        // Row 3: dac, menu (stretched to fill the row)
        ButtonDef(10, "DAC",        0x0113, 10, R.drawable.ic_tune, spanSize = 2),
        ButtonDef(11, "Menu ▶",    0x0107, 11, R.drawable.ic_navigate_next, spanSize = 2),
        // Remaining buttons
        ButtonDef(7,  "Cover",      0x0119,  7, R.drawable.ic_album),
        ButtonDef(12, "Meter",      0x0115, 12, R.drawable.ic_equalizer),
        ButtonDef(13, "Vol –",      0x0110, -1, R.drawable.ic_remove_circle),
        ButtonDef(14, "Vol +",      0x0111, -1, R.drawable.ic_add_circle),
        ButtonDef(15, "Bright",     0x0116, 15, R.drawable.ic_brightness),
        // Last row: power by itself, full width
        ButtonDef(0,  "Power",      0x0001, -1, R.drawable.ic_power, spanSize = 4, backgroundColorRes = R.color.btn_bg_power)
    )
}

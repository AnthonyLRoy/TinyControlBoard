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
        // Row 1: skip back, prev, next, skip — transport group
        ButtonDef(4,  "◀◀ Skip",   0x0105,  4, R.drawable.ic_fast_rewind, backgroundColorRes = R.color.btn_bg_transport),
        ButtonDef(1,  "Prev",       0x0101,  1, R.drawable.ic_skip_previous, backgroundColorRes = R.color.btn_bg_transport),
        ButtonDef(2,  "Next",       0x0100,  2, R.drawable.ic_skip_next, backgroundColorRes = R.color.btn_bg_transport),
        ButtonDef(3,  "Skip ▶▶",   0x0104,  3, R.drawable.ic_fast_forward, backgroundColorRes = R.color.btn_bg_transport),
        // Row 2: stop, play/pause — transport group; repeat, shuffle — utility group
        ButtonDef(6,  "Stop",       0x0103,  6, R.drawable.ic_stop, backgroundColorRes = R.color.btn_bg_transport),
        ButtonDef(5,  "Play/Pause", 0x0102,  5, R.drawable.ic_play_pause, backgroundColorRes = R.color.btn_bg_transport),
        ButtonDef(8,  "Repeat",     0x011C,  8, R.drawable.ic_repeat, backgroundColorRes = R.color.btn_bg_utility),
        ButtonDef(9,  "Shuffle",    0x011F,  9, R.drawable.ic_shuffle, backgroundColorRes = R.color.btn_bg_utility),
        // Row 3: dac, menu (stretched to fill the row) — navigation group
        ButtonDef(10, "DAC",        0x0113, 10, R.drawable.ic_tune, spanSize = 2, backgroundColorRes = R.color.btn_bg_nav),
        ButtonDef(11, "Menu ▶",    0x0107, 11, R.drawable.ic_navigate_next, spanSize = 2, backgroundColorRes = R.color.btn_bg_nav),
        // Row 4: cover, meter (stretched to fill the row) — navigation group
        ButtonDef(7,  "Cover",      0x0119,  7, R.drawable.ic_album, spanSize = 2, backgroundColorRes = R.color.btn_bg_nav),
        ButtonDef(12, "Meter",      0x0115, 12, R.drawable.ic_equalizer, spanSize = 2, backgroundColorRes = R.color.btn_bg_nav),
        // Row 5: vol down/up + brightness (stretched) — utility group
        ButtonDef(13, "Vol –",      0x0110, -1, R.drawable.ic_remove_circle, backgroundColorRes = R.color.btn_bg_utility),
        ButtonDef(14, "Vol +",      0x0111, -1, R.drawable.ic_add_circle, backgroundColorRes = R.color.btn_bg_utility),
        ButtonDef(15, "Bright",     0x0116, 15, R.drawable.ic_brightness, spanSize = 2, backgroundColorRes = R.color.btn_bg_utility)
        // Power (commandId 0x0001) moved out of the grid — it's now a small icon button in the
        // top status strip (see activity_main.xml / MainActivity) so it can't be pressed by
        // accident alongside the frequently-used transport buttons.
    )
}

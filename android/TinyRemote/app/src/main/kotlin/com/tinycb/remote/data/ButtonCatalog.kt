package com.tinycb.remote.data

import com.tinycb.remote.R
import com.tinycb.remote.model.ButtonDef
import com.tinycb.remote.model.GridItem

/**
 * Hardcoded catalog of all 16 physical buttons on the TinyControlBoard.
 * commandId values match the CommandId enum in lib/protocol/uartProtocol.hpp.
 * bitmaskBit is the bit index in the 16-bit buttonLedBitmask (-1 = no LED).
 */
object ButtonCatalog {
    val gridItems: List<GridItem> = buildList {
        add(GridItem.Header("TRANSPORT"))
        // Row 1: skip back, prev, next, skip forward
        add(GridItem.Button(ButtonDef(4,  "◀◀ Skip",    0x0105,  4, R.drawable.ic_fast_rewind)))
        add(GridItem.Button(ButtonDef(1,  "Prev",        0x0101,  1, R.drawable.ic_skip_previous)))
        add(GridItem.Button(ButtonDef(2,  "Next",        0x0100,  2, R.drawable.ic_skip_next)))
        add(GridItem.Button(ButtonDef(3,  "Skip ▶▶",    0x0104,  3, R.drawable.ic_fast_forward)))
        // Row 2: stop, play/pause, repeat, shuffle
        add(GridItem.Button(ButtonDef(6,  "Stop",        0x0103,  6, R.drawable.ic_stop)))
        add(GridItem.Button(ButtonDef(5,  "Play",       0x0102,  5, R.drawable.ic_play,       isPrimary = true, isToggle = true)))
        add(GridItem.Button(ButtonDef(8,  "Repeat",      0x011C,  8, R.drawable.ic_repeat)))
        add(GridItem.Button(ButtonDef(9,  "Shuffle",     0x011F,  9, R.drawable.ic_shuffle)))

        add(GridItem.Header("NAVIGATION"))
        add(GridItem.Button(ButtonDef(10, "DAC",         0x0113, 10, R.drawable.ic_tune,          spanSize = 2)))
        add(GridItem.Button(ButtonDef(11, "Menu ▶",     0x0107, 11, R.drawable.ic_navigate_next, spanSize = 2)))
        add(GridItem.Button(ButtonDef(7,  "Cover",       0x0119,  7, R.drawable.ic_album,         spanSize = 2)))
        add(GridItem.Button(ButtonDef(12, "Meter",       0x0115, 12, R.drawable.ic_equalizer,     spanSize = 2)))

        add(GridItem.Header("DISPLAY"))
        add(GridItem.Stepper("Brightness", R.drawable.ic_brightness, 0x0121, 0x0120))
        // Power is in the status strip — see activity_main.xml / MainActivity
        add(GridItem.Button(ButtonDef(15, "Display Off", 0x0114, 15, R.drawable.ic_brightness,   spanSize = 4)))
    }
}

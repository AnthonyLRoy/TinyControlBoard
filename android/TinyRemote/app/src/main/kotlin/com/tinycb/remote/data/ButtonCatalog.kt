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
    const val CMD_PREV        = 0x0101
    const val CMD_NEXT        = 0x0100
    const val CMD_PLAY        = 0x0102
    const val CMD_STOP        = 0x0103
    const val CMD_SKIP_BACK   = 0x0105
    const val CMD_SKIP_FWD    = 0x0104
    const val CMD_DISPLAY_OFF = 0x0114
    const val CMD_VIEW_MENU   = 0x0107
    const val CMD_PLAYLIST    = -2

    val gridItems: List<GridItem> = buildList {
        add(GridItem.Header("TRANSPORT"))
        // Row 1: skip back, prev, next, skip forward
        add(GridItem.Button(ButtonDef(4,  "◀◀ Skip",    CMD_SKIP_BACK,  4, R.drawable.ic_fast_rewind)))
        add(GridItem.Button(ButtonDef(1,  "Prev",        CMD_PREV,       1, R.drawable.ic_skip_previous)))
        add(GridItem.Button(ButtonDef(2,  "Next",        CMD_NEXT,       2, R.drawable.ic_skip_next)))
        add(GridItem.Button(ButtonDef(3,  "Skip ▶▶",    CMD_SKIP_FWD,    3, R.drawable.ic_fast_forward)))
        // Row 2: stop, play/pause, repeat, shuffle
        add(GridItem.Button(ButtonDef(6,  "Stop",        CMD_STOP,       -1, R.drawable.ic_stop)))
        add(GridItem.Button(ButtonDef(5,  "Play",       CMD_PLAY,       5, R.drawable.ic_play,       isPrimary = true, isToggle = true)))
        add(GridItem.Button(ButtonDef(8,  "Repeat",      0x011C,  8, R.drawable.ic_repeat)))
        add(GridItem.Button(ButtonDef(9,  "Shuffle",     0x011F,  9, R.drawable.ic_shuffle)))

        add(GridItem.Header("DISPLAY"))
        add(GridItem.Button(ButtonDef(11, "Menu ▶",     CMD_VIEW_MENU, 11, R.drawable.ic_navigate_next, spanSize = 2)))
        add(GridItem.Button(ButtonDef(17, "Playlist",   CMD_PLAYLIST, -1, R.drawable.ic_queue_music,    spanSize = 2)))
        add(GridItem.Stepper("Brightness", R.drawable.ic_brightness, 0x0121, 0x0120))
        add(GridItem.Button(ButtonDef(15, "Display",     CMD_DISPLAY_OFF, 6, R.drawable.ic_brightness, spanSize = 2, showLabel = false, isToggle = true)))
        add(GridItem.Button(ButtonDef(7,  "Cover",       0x0119,  7, R.drawable.ic_album,         spanSize = 2)))
        add(GridItem.Button(ButtonDef(12, "Meter",       0x0115, 12, R.drawable.ic_meter,         spanSize = 2)))

        add(GridItem.Header("OPTIONS"))
        add(GridItem.Button(ButtonDef(10, "DAC",         0x0113, 10, R.drawable.ic_tune,          spanSize = 2)))
        add(GridItem.Button(ButtonDef(16, "Library",     CMD_LIBRARY, -1, R.drawable.ic_folder,    spanSize = 2)))
    }

    const val CMD_LIBRARY = -1
}


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
    const val CMD_REPEAT      = 0x011C
    const val CMD_SHUFFLE     = 0x011F
    const val CMD_BRIGHT_DOWN = 0x0121
    const val CMD_BRIGHT_UP   = 0x0120
    const val CMD_DISPLAY_OFF = 0x0114
    const val CMD_VIEW_MENU   = 0x0107

    // Transport (prev/play/next), shuffle/repeat/menu and brightness controls are rendered as
    // fixed views in activity_main.xml rather than grid items; only the remaining
    // secondary options live in this scrollable catalog.
    val gridItems: List<GridItem> = buildList {
        add(GridItem.Button(ButtonDef(7,  "Cover",       0x0119,  7, R.drawable.ic_album,         spanSize = 1, iconSizeDp = 48, isToggle = true)))
        add(GridItem.Button(ButtonDef(12, "Meter",       0x0115, 12, R.drawable.ic_radio_wave,    spanSize = 1, iconSizeDp = 48, isToggle = true)))
        add(GridItem.Button(ButtonDef(10, "DAC",         0x0113, 10, R.drawable.ic_tune,          spanSize = 1, iconSizeDp = 48, isToggle = true)))
    }

    const val CMD_LIBRARY = -1
}


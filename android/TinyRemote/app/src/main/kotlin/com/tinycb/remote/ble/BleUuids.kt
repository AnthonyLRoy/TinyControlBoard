package com.tinycb.remote.ble

import java.util.UUID

object BleUuids {
    val SERVICE: UUID    = UUID.fromString("4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d0e")
    val CMD_CHAR: UUID   = UUID.fromString("4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d01")
    val STATUS_CHAR: UUID = UUID.fromString("4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d02")
    // Standard Client Characteristic Configuration Descriptor (CCCD)
    val CCCD: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
}

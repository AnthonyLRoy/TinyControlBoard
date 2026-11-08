package com.tinycb.remote.ui

import com.tinycb.remote.ble.ConnectionState
import com.tinycb.remote.model.BoardStatus

internal fun controlsAvailable(connection: ConnectionState, status: BoardStatus?): Boolean =
    connection is ConnectionState.Connected && status?.powerStateName == "ON"

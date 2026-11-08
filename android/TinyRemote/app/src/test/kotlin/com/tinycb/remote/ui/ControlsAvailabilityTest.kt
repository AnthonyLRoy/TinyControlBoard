package com.tinycb.remote.ui

import com.tinycb.remote.ble.ConnectionState
import com.tinycb.remote.model.BoardStatus
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ControlsAvailabilityTest {
    @Test
    fun onlyConnectedAndConfirmedOnEnablesControls() {
        val connected = ConnectionState.Connected()
        assertTrue(controlsAvailable(connected, BoardStatus("ON", 0)))

        assertFalse(controlsAvailable(connected, null))
        for (state in listOf("UNKNOWN", "OFF", "SLEEP", "DEEP SLEEP", "TURNING ON", "GOING TO SLEEP", "GOING INTO DEEP SLEEP", "SHUTTING DOWN")) {
            assertFalse(state, controlsAvailable(connected, BoardStatus(state, 0)))
        }
        for (connection in listOf(ConnectionState.Idle, ConnectionState.Scanning, ConnectionState.Disconnected, ConnectionState.Error("lost"))) {
            assertFalse(controlsAvailable(connection, BoardStatus("ON", 0)))
        }
    }
}

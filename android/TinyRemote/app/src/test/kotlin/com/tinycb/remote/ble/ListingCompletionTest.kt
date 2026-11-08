package com.tinycb.remote.ble

import com.tinycb.remote.model.LibraryEntry
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ListingCompletionTest {
    private fun entry(index: Int, total: Int) = LibraryEntry(index, total, false, "playlist")

    @Test
    fun waitsForEveryDistinctEntry() {
        assertFalse(BleProtocol.isCompleteListing(null))
        assertFalse(BleProtocol.isCompleteListing(listOf(entry(0, 2))))
        assertFalse(BleProtocol.isCompleteListing(listOf(entry(0, 2), entry(0, 2))))
        assertTrue(BleProtocol.isCompleteListing(listOf(entry(0, 2), entry(1, 2))))
        assertTrue(BleProtocol.isCompleteListing(emptyList()))
    }
}

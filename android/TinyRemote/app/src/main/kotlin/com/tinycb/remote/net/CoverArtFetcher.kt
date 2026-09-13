package com.tinycb.remote.net

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.net.HttpURLConnection
import java.net.Socket
import java.net.URL
import java.net.URLEncoder

/**
 * Fetches the currently playing track's cover art directly from moOde/MPD —
 * independent of the BLE control link, which never carries image data.
 */
object CoverArtFetcher {

    private const val TAG = "CoverArtFetcher"

    // TODO: point this at your Raspberry Pi's LAN IP or hostname (e.g. "moode.local").
    const val DEFAULT_BOARD_HOST = "192.168.0.10"

    private const val MPD_PORT = 6600
    private const val CONNECT_TIMEOUT_MS = 3000
    private const val READ_TIMEOUT_MS = 5000

    suspend fun fetchCoverArt(host: String = DEFAULT_BOARD_HOST): Bitmap? = withContext(Dispatchers.IO) {
        try {
            val filePath = fetchCurrentSongFilePath(host) ?: return@withContext null
            fetchCoverArtBitmap(host, filePath)
        } catch (e: Exception) {
            Log.w(TAG, "Cover art fetch failed: ${e.message}")
            null
        }
    }

    // moOde's own get_currentsong HTTP endpoint depends on a cache file that isn't always kept
    // in sync (worker.php); querying MPD's "currentsong" over its own TCP protocol is reliable.
    private fun fetchCurrentSongFilePath(host: String): String? {
        Socket().use { socket ->
            socket.connect(java.net.InetSocketAddress(host, MPD_PORT), CONNECT_TIMEOUT_MS)
            socket.soTimeout = READ_TIMEOUT_MS
            val reader = socket.getInputStream().bufferedReader()
            val writer = socket.getOutputStream().bufferedWriter()
            reader.readLine() // MPD banner, e.g. "OK MPD 0.24.0"
            writer.write("currentsong\n")
            writer.flush()
            var filePath: String? = null
            while (true) {
                val line = reader.readLine() ?: break
                if (line == "OK" || line.startsWith("ACK ")) break
                if (line.startsWith("file: ")) filePath = line.removePrefix("file: ")
            }
            return filePath
        }
    }

    private fun fetchCoverArtBitmap(host: String, filePath: String): Bitmap? {
        val encodedPath = filePath.split("/").joinToString("/") { segment ->
            URLEncoder.encode(segment, "UTF-8").replace("+", "%20")
        }
        val url = URL("http://$host/coverart.php/$encodedPath")
        val connection = url.openConnection() as HttpURLConnection
        connection.connectTimeout = CONNECT_TIMEOUT_MS
        connection.readTimeout = READ_TIMEOUT_MS
        return try {
            if (connection.responseCode != HttpURLConnection.HTTP_OK) return null
            connection.inputStream.use { BitmapFactory.decodeStream(it) }
        } finally {
            connection.disconnect()
        }
    }
}


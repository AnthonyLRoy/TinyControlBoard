package com.tinycb.remote.net

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import org.json.JSONObject
import com.tinycb.remote.model.RemoteTrackMetadata
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.net.HttpURLConnection
import java.net.Socket
import java.net.URL
import java.net.URLDecoder
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
    private const val CURRENT_SONG_PATH = "/command/index.php?cmd=get_currentsong"

    suspend fun fetchCoverArt(host: String = DEFAULT_BOARD_HOST): Bitmap? = withContext(Dispatchers.IO) {
        try {
            val metadata = fetchCurrentSongMetadataInternal(host)
            if (!metadata?.coverUrl.isNullOrEmpty()) fetchBitmap(host, metadata?.coverUrl!!)
            else fetchCurrentSongFilePath(host)?.let { fetchCoverArtBitmap(host, it) }
        } catch (e: Exception) {
            Log.w(TAG, "Cover art fetch failed: ${e.message}")
            null
        }
    }

    suspend fun fetchCurrentSongMetadata(host: String = DEFAULT_BOARD_HOST): RemoteTrackMetadata? =
        withContext(Dispatchers.IO) {
            try {
                fetchCurrentSongMetadataInternal(host)
            } catch (e: Exception) {
                Log.w(TAG, "Current-song metadata fetch failed: ${e.message}")
                null
            }
        }

    // moOde's get_currentsong command returns JSON generated from currentsong.txt.
    // Older builds may expose the backing file format directly, so accept both forms.
    private fun fetchCurrentSongMetadataInternal(host: String): RemoteTrackMetadata? {
        val connection = URL("http://$host$CURRENT_SONG_PATH").openConnection() as HttpURLConnection
        connection.connectTimeout = CONNECT_TIMEOUT_MS
        connection.readTimeout = READ_TIMEOUT_MS
        connection.useCaches = false
        connection.setRequestProperty("Cache-Control", "no-cache")
        return try {
            if (connection.responseCode != HttpURLConnection.HTTP_OK) return null
            val body = connection.inputStream.bufferedReader().use { it.readText() }
            parseCurrentSong(body)
        } finally {
            connection.disconnect()
        }
    }

    private fun parseCurrentSong(body: String): RemoteTrackMetadata? {
        val trimmed = body.trim()
        if (trimmed.startsWith("{")) {
            val json = JSONObject(trimmed)
            val title = json.optString("title").ifBlank { json.optString("name") }
            val artist = json.optString("artist")
            val album = json.optString("album")
            val file = json.optString("file")
            if (title.isBlank() && artist.isBlank() && album.isBlank() && file.isBlank()) return null
            return RemoteTrackMetadata(title, artist, album, file, json.optString("coverurl").ifBlank { null })
        }

        val fields = trimmed.lineSequence()
            .mapNotNull { line -> line.split('=', limit = 2).takeIf { it.size == 2 } }
            .associate { it[0].lowercase() to it[1] }
        val title = fields["title"].orEmpty().ifBlank { fields["name"].orEmpty() }
        val artist = fields["artist"].orEmpty()
        val album = fields["album"].orEmpty()
        val file = fields["file"].orEmpty()
        if (title.isBlank() && artist.isBlank() && album.isBlank() && file.isBlank()) return null
        return RemoteTrackMetadata(title, artist, album, file, fields["coverurl"])
    }

    private fun parseCoverUrl(body: String): String? {
        val trimmed = body.trim()
        if (trimmed.startsWith("{")) {
            return JSONObject(trimmed).optString("coverurl").takeIf { it.isNotEmpty() }
        }
        return trimmed.lineSequence()
            .firstOrNull { it.startsWith("coverurl=") }
            ?.substringAfter('=')
            ?.takeIf { it.isNotEmpty() }
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
        return fetchBitmap(host, "/coverart.php/$encodedPath")
    }

    private fun fetchBitmap(host: String, imagePath: String): Bitmap? {
        val imageUrl = if (imagePath.startsWith("http://") || imagePath.startsWith("https://")) {
            imagePath
        } else {
            // moOde stores local radio-logo paths as one rawurlencoded string,
            // e.g. imagesw%2Fradio-logos%2FStation.jpg. Keep coverart.php paths
            // encoded because their encoded slashes identify the source file.
            val decodedPath = if (imagePath.contains("%2F", ignoreCase = true)) {
                URLDecoder.decode(imagePath, "UTF-8")
            } else {
                imagePath
            }
            "http://$host/${decodedPath.trimStart('/')}"
        }
        val url = URL(imageUrl)
        val connection = url.openConnection() as HttpURLConnection
        connection.connectTimeout = CONNECT_TIMEOUT_MS
        connection.readTimeout = READ_TIMEOUT_MS
        connection.useCaches = false
        connection.setRequestProperty("Cache-Control", "no-cache")
        return try {
            if (connection.responseCode != HttpURLConnection.HTTP_OK) return null
            connection.inputStream.use { BitmapFactory.decodeStream(it) }
        } finally {
            connection.disconnect()
        }
    }
}


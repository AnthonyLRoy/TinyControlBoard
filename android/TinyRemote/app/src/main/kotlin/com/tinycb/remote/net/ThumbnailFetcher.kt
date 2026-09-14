package com.tinycb.remote.net

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import android.util.LruCache
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.net.HttpURLConnection
import java.net.URL

/**
 * Fetches moOde's pre-generated album-directory thumbnail (thmcache) directly over HTTP,
 * given the MD5 hash the RPi computed for that directory — avoids the larger coverart.php
 * full-size image path entirely for grid/list-style thumbnails.
 */
object ThumbnailFetcher {

    private const val TAG = "ThumbnailFetcher"
    private const val CONNECT_TIMEOUT_MS = 3000
    private const val READ_TIMEOUT_MS = 5000

    // Thumbnails are immutable per album directory hash, safe to cache for the app's lifetime.
    private val cache = LruCache<String, Bitmap>(64)

    suspend fun fetch(hash: String, host: String = CoverArtFetcher.DEFAULT_BOARD_HOST): Bitmap? =
        withContext(Dispatchers.IO) {
            if (hash.isEmpty()) return@withContext null
            cache.get(hash)?.let { return@withContext it }

            val url = URL("http://$host/imagesw/thmcache/$hash.jpg")
            val connection = url.openConnection() as HttpURLConnection
            connection.connectTimeout = CONNECT_TIMEOUT_MS
            connection.readTimeout = READ_TIMEOUT_MS
            try {
                if (connection.responseCode != HttpURLConnection.HTTP_OK) return@withContext null
                val bitmap = connection.inputStream.use { BitmapFactory.decodeStream(it) }
                bitmap?.also { cache.put(hash, it) }
            } catch (e: Exception) {
                Log.w(TAG, "Thumbnail fetch failed for hash=$hash: ${e.message}")
                null
            } finally {
                connection.disconnect()
            }
        }
}

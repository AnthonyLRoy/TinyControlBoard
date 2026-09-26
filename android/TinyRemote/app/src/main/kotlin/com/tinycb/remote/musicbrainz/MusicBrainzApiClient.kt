package com.tinycb.remote.musicbrainz

import android.content.Context
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URLEncoder
import java.net.URL
import java.io.InputStreamReader
import java.nio.charset.StandardCharsets

class MusicBrainzApiClient(context: Context) {
    private val cache = context.getSharedPreferences("musicbrainz_cache", Context.MODE_PRIVATE)
    private val requestMutex = Mutex()
    private var lastRequestAt = 0L
    private var blockedUntil = 0L

    suspend fun get(path: String): JSONObject = withContext(Dispatchers.IO) {
        val key = "json:$path"
        val cached = cache.getString(key, null)
        val cachedAt = cache.getLong("$key:at", 0L)
        if (cached != null && System.currentTimeMillis() - cachedAt < CACHE_TTL_MS) return@withContext JSONObject(cached)

        requestMutex.withLock {
            val refreshed = cache.getString(key, null)
            val refreshedAt = cache.getLong("$key:at", 0L)
            if (refreshed != null && System.currentTimeMillis() - refreshedAt < CACHE_TTL_MS) {
                return@withLock JSONObject(refreshed)
            }
            val waitMs = (MIN_INTERVAL_MS - (System.currentTimeMillis() - lastRequestAt)).coerceAtLeast(0L)
            val retryWaitMs = (blockedUntil - System.currentTimeMillis()).coerceAtLeast(0L)
            if (maxOf(waitMs, retryWaitMs) > 0) kotlinx.coroutines.delay(maxOf(waitMs, retryWaitMs))
            val result = request(path)
            lastRequestAt = System.currentTimeMillis()
            cache.edit().putString(key, result.toString()).putLong("$key:at", lastRequestAt).apply()
            result
        }
    }

    private fun request(path: String): JSONObject {
        val connection = URL("https://musicbrainz.org/ws/2/$path").openConnection() as HttpURLConnection
        connection.connectTimeout = 10_000
        connection.readTimeout = 15_000
        connection.requestMethod = "GET"
        connection.instanceFollowRedirects = true
        connection.setRequestProperty("Accept", "application/json")
        connection.setRequestProperty("Accept-Encoding", "identity")
        connection.setRequestProperty("User-Agent", "DanStreamerAndroid/1.0 (https://github.com/tinycontrolboard)")
        return try {
            when (val code = connection.responseCode) {
                HttpURLConnection.HTTP_OK -> JSONObject(
                    InputStreamReader(connection.inputStream, StandardCharsets.UTF_8).use { it.readText() }
                )
                HttpURLConnection.HTTP_UNAVAILABLE -> {
                    val retryAfter = connection.getHeaderField("Retry-After")?.toLongOrNull()
                    blockedUntil = System.currentTimeMillis() + ((retryAfter ?: 1L).coerceAtMost(60L) * 1000L)
                    throw MusicBrainzNetworkException("MusicBrainz is temporarily unavailable. Please try again.")
                }
                else -> throw MusicBrainzNetworkException("MusicBrainz request failed (HTTP $code).")
            }
        } catch (e: MusicBrainzNetworkException) {
            throw e
        } catch (e: Exception) {
            Log.e(TAG, "MusicBrainz request failed for $path", e)
            throw MusicBrainzNetworkException("Unable to reach MusicBrainz.", e)
        } finally {
            connection.disconnect()
        }
    }

    companion object {
        private const val TAG = "MusicBrainzApiClient"
        private const val MIN_INTERVAL_MS = 1_050L
        private const val CACHE_TTL_MS = 7L * 24 * 60 * 60 * 1000

        fun query(value: String): String = value
            .replace("\\", "\\\\")
            .replace(Regex("([+\\-!(){}\\[\\]^\"~*?:&|])"), "\\\\$1")
            .trim()

        fun encoded(value: String): String = URLEncoder.encode(value, Charsets.UTF_8.name())
    }
}
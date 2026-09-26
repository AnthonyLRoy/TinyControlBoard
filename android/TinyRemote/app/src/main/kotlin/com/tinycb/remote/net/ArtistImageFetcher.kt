package com.tinycb.remote.net

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.LruCache
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

object ArtistImageFetcher {
    private const val SUMMARY_URL = "https://en.wikipedia.org/api/rest_v1/page/summary/"
    private const val USER_AGENT = "DanStreamerAndroid/1.0 (https://github.com/tinycontrolboard)"
    private val cache = LruCache<String, Bitmap>(8)

    data class ArtistSupplement(val image: Bitmap?, val extract: String?)

    suspend fun fetchSupplement(context: Context, artistName: String): ArtistSupplement =
        kotlinx.coroutines.withContext(kotlinx.coroutines.Dispatchers.IO) {
            if (artistName.isBlank()) return@withContext ArtistSupplement(null, null)
            try {
                val pageTitle = URLEncoder.encode(artistName, Charsets.UTF_8.name()).replace("+", "_")
                val summary = requestJson("$SUMMARY_URL$pageTitle")
                val source = summary.optJSONObject("thumbnail")?.optString("source").orEmpty()
                val image = cache.get(artistName) ?: source.takeIf { it.isNotBlank() }?.let { requestBitmap(it) }
                    ?.also { cache.put(artistName, it) }
                ArtistSupplement(image, summary.optString("extract").ifBlank { null })
            } catch (_: Exception) {
                ArtistSupplement(null, null)
            }
        }

    suspend fun fetch(context: Context, artistName: String): Bitmap? = kotlinx.coroutines.withContext(kotlinx.coroutines.Dispatchers.IO) {
        if (artistName.isBlank()) return@withContext null
        cache.get(artistName)?.let { return@withContext it }
        try {
            val pageTitle = URLEncoder.encode(artistName, Charsets.UTF_8.name()).replace("+", "_")
            val summary = requestJson("$SUMMARY_URL$pageTitle")
            val source = summary.optJSONObject("thumbnail")?.optString("source").orEmpty()
            if (source.isBlank()) return@withContext null
            val bitmap = requestBitmap(source)
            bitmap?.also { cache.put(artistName, it) }
        } catch (_: Exception) {
            null
        }
    }

    private fun requestJson(url: String): JSONObject {
        val connection = URL(url).openConnection() as HttpURLConnection
        connection.connectTimeout = 8_000
        connection.readTimeout = 10_000
        connection.setRequestProperty("Accept", "application/json")
        connection.setRequestProperty("User-Agent", USER_AGENT)
        return try {
            if (connection.responseCode != HttpURLConnection.HTTP_OK) return JSONObject()
            JSONObject(connection.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() })
        } finally {
            connection.disconnect()
        }
    }

    private fun requestBitmap(url: String): Bitmap? {
        val connection = URL(url).openConnection() as HttpURLConnection
        connection.connectTimeout = 8_000
        connection.readTimeout = 10_000
        connection.setRequestProperty("User-Agent", USER_AGENT)
        return try {
            if (connection.responseCode != HttpURLConnection.HTTP_OK) null
            else connection.inputStream.use { BitmapFactory.decodeStream(it) }
        } finally {
            connection.disconnect()
        }
    }
}
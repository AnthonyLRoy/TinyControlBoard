package com.tinycb.remote.musicbrainz

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.max

class MusicBrainzRepository(context: Context) {
    private val api = MusicBrainzApiClient(context)

    suspend fun artist(name: String): MusicBrainzLookup<MusicBrainzArtist> {
        val search = api.get("artist/?query=${MusicBrainzApiClient.encoded("artist:${MusicBrainzApiClient.query(name)}")}&fmt=json&limit=8")
        val candidates = search.optJSONArray("artists").toCandidates()
        val selected = select(name, candidates) ?: throw MusicBrainzNoMatchException()
        if (selected is Selection.Many) return MusicBrainzLookup.Choose(selected.items)
        val id = (selected as Selection.One).item.id
        return MusicBrainzLookup.Ready(parseArtist(api.get("artist/$id?inc=aliases+tags+genres+url-rels&fmt=json")))
    }

    suspend fun artistById(id: String): MusicBrainzArtist = parseArtist(api.get("artist/$id?inc=aliases+tags+genres+url-rels&fmt=json"))

    suspend fun releaseGroup(artist: String, album: String): MusicBrainzLookup<MusicBrainzReleaseGroup> {
        val query = if (artist.isBlank()) {
            "releasegroup:${MusicBrainzApiClient.query(album)}"
        } else {
            "releasegroup:${MusicBrainzApiClient.query(album)} AND artist:${MusicBrainzApiClient.query(artist)}"
        }
        val search = api.get("release-group/?query=${MusicBrainzApiClient.encoded(query)}&fmt=json&limit=8")
        val candidates = search.optJSONArray("release-groups").toCandidates()
        val selected = select(album, candidates) ?: throw MusicBrainzNoMatchException()
        if (selected is Selection.Many) return MusicBrainzLookup.Choose(selected.items)
        val id = (selected as Selection.One).item.id
        return MusicBrainzLookup.Ready(enrichReleaseGroup(parseReleaseGroup(api.get("release-group/$id?inc=releases+tags+genres+url-rels&fmt=json"))))
    }

    suspend fun releaseGroupById(id: String): MusicBrainzReleaseGroup {
        return enrichReleaseGroup(parseReleaseGroup(api.get("release-group/$id?inc=releases+tags+genres+url-rels&fmt=json")))
    }

    private suspend fun enrichReleaseGroup(group: MusicBrainzReleaseGroup): MusicBrainzReleaseGroup {
        val releaseId = group.release?.id ?: return group
        val detail = api.get("release/$releaseId?inc=recordings+labels+artist-credits&fmt=json")
        return group.copy(release = parseRelease(detail))
    }

    private fun select(query: String, candidates: List<MusicBrainzCandidate>): Selection? {
        if (candidates.isEmpty()) return null
        val ranked = candidates.sortedByDescending { it.score * 0.7 + similarity(query, it.name) * 30.0 }
        val best = ranked.first()
        return if (best.score >= 90 && similarity(query, best.name) >= 0.72 &&
            (ranked.size == 1 || best.score - ranked[1].score >= 8)) Selection.One(best)
        else Selection.Many(ranked.take(5))
    }

    private fun similarity(a: String, b: String): Double {
        val left = a.lowercase().replace(Regex("[^a-z0-9]"), "")
        val right = b.lowercase().replace(Regex("[^a-z0-9]"), "")
        if (left == right) return 1.0
        val distance = Array(right.length + 1) { it }
        for (i in 1..left.length) {
            var diagonal = distance[0]
            distance[0] = i
            for (j in 1..right.length) {
                val above = distance[j]
                distance[j] = if (left[i - 1] == right[j - 1]) diagonal else 1 + minOf(diagonal, above, distance[j - 1])
                diagonal = above
            }
        }
        return 1.0 - distance[right.length].toDouble() / max(left.length, right.length).coerceAtLeast(1)
    }

    private fun parseArtist(json: JSONObject): MusicBrainzArtist {
        val life = json.optJSONObject("life-span")
        return MusicBrainzArtist(
            json.optString("id"), json.optString("name"), json.optString("type"), json.optString("country"),
            listOfNotNull(life?.optString("begin").orEmpty().ifBlank { null }, life?.optString("end").orEmpty().ifBlank { null })
                .joinToString(" - ").ifBlank { "" },
            json.optJSONArray("genres").toNames("name"), json.optJSONArray("tags").toNames("name").take(8),
            json.optJSONArray("aliases").toNames("name").take(12), json.optString("disambiguation"), parseLinks(json)
        )
    }

    private fun parseReleaseGroup(json: JSONObject): MusicBrainzReleaseGroup {
        val releases = json.optJSONArray("releases") ?: JSONArray()
        val release = (0 until releases.length()).map { releases.optJSONObject(it) }
            .filterNotNull().sortedWith(compareBy({ it.optString("status") != "Official" }, { it.optString("date") })).firstOrNull()
            ?.let { parseRelease(it) }
        return MusicBrainzReleaseGroup(
            json.optString("id"), json.optString("title"), json.optJSONArray("artist-credit").artistName(),
            json.optString("first-release-date"), json.optString("primary-type"), json.optJSONArray("secondary-types").toStrings(),
            json.optJSONArray("genres").toNames("name"), json.optJSONArray("tags").toNames("name").take(8),
            json.optString("disambiguation"), parseLinks(json), release
        )
    }

    private fun parseRelease(json: JSONObject): MusicBrainzRelease = MusicBrainzRelease(
        json.optString("id"), json.optString("title"), json.optString("date"), json.optString("country"), json.optString("status"),
        json.optJSONArray("label-info")?.optJSONObject(0)?.optJSONObject("label")?.optString("name").orEmpty(),
        json.optJSONArray("media")?.optJSONObject(0)?.optJSONArray("tracks").toTracks()
    )

    private fun parseLinks(json: JSONObject): List<Pair<String, String>> = json.optJSONArray("relations")?.let { relations ->
        (0 until relations.length()).mapNotNull { i ->
            val relation = relations.optJSONObject(i) ?: return@mapNotNull null
            val target = relation.optString("target").ifBlank { return@mapNotNull null }
            val type = relation.optString("type").replace('-', ' ').replaceFirstChar { it.uppercase() }
            type to target
        }.distinctBy { it.second }.take(8)
    }.orEmpty()

    private sealed class Selection { data class One(val item: MusicBrainzCandidate) : Selection(); data class Many(val items: List<MusicBrainzCandidate>) : Selection() }
}

private fun JSONArray?.toCandidates(): List<MusicBrainzCandidate> = if (this == null) emptyList() else
    (0 until length()).mapNotNull { i -> optJSONObject(i)?.let { MusicBrainzCandidate(it.optString("id"), it.optString("name").ifBlank { it.optString("title") }, it.optInt("score"), it.optString("country"), it.optString("type"), it.optString("disambiguation")) } }

private fun JSONArray?.toNames(key: String): List<String> = if (this == null) emptyList() else (0 until length()).mapNotNull { optJSONObject(it)?.optString(key)?.ifBlank { null } }.distinct()
private fun JSONArray?.toStrings(): List<String> = if (this == null) emptyList() else (0 until length()).mapNotNull { optString(it).ifBlank { null } }
private fun JSONArray?.artistName(): String = if (this == null) "" else (0 until length()).mapNotNull { optJSONObject(it)?.optString("name") }.joinToString(", ")
private fun JSONArray?.toTracks(): List<MusicBrainzTrack> = if (this == null) emptyList() else (0 until length()).mapNotNull { optJSONObject(it)?.let { track -> MusicBrainzTrack(track.optString("position"), track.optString("title"), track.optInt("length").takeIf { it > 0 }) } }
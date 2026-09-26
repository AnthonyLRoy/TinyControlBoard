package com.tinycb.remote.musicbrainz

data class MusicBrainzCandidate(
    val id: String,
    val name: String,
    val score: Int,
    val country: String = "",
    val type: String = "",
    val disambiguation: String = ""
)

data class MusicBrainzArtist(
    val id: String,
    val name: String,
    val type: String,
    val country: String,
    val lifeSpan: String,
    val genres: List<String>,
    val tags: List<String>,
    val aliases: List<String>,
    val disambiguation: String,
    val links: List<Pair<String, String>>
)

data class MusicBrainzTrack(val position: String, val title: String, val durationMs: Int?)

data class MusicBrainzRelease(
    val id: String,
    val title: String,
    val date: String,
    val country: String,
    val status: String,
    val label: String,
    val tracks: List<MusicBrainzTrack>
)

data class MusicBrainzReleaseGroup(
    val id: String,
    val title: String,
    val artist: String,
    val firstReleaseDate: String,
    val primaryType: String,
    val secondaryTypes: List<String>,
    val genres: List<String>,
    val tags: List<String>,
    val disambiguation: String,
    val links: List<Pair<String, String>>,
    val release: MusicBrainzRelease?
)

sealed class MusicBrainzLookup<out T> {
    data class Ready<T>(val value: T) : MusicBrainzLookup<T>()
    data class Choose<T>(val candidates: List<MusicBrainzCandidate>) : MusicBrainzLookup<T>()
}

class MusicBrainzNetworkException(message: String, cause: Throwable? = null) : Exception(message, cause)
class MusicBrainzNoMatchException : Exception()
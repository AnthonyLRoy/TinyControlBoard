package com.tinycb.remote.musicbrainz

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.viewModelScope
import com.tinycb.remote.model.RemoteTrackMetadata
import com.tinycb.remote.net.CoverArtFetcher
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed class MusicBrainzScreenState {
    data object Loading : MusicBrainzScreenState()
    data class ArtistReady(val value: MusicBrainzArtist, val metadata: RemoteTrackMetadata) : MusicBrainzScreenState()
    data class AlbumReady(val value: MusicBrainzReleaseGroup, val metadata: RemoteTrackMetadata) : MusicBrainzScreenState()
    data class Choose(val candidates: List<MusicBrainzCandidate>) : MusicBrainzScreenState()
    data class Error(val message: String, val retryable: Boolean) : MusicBrainzScreenState()
}

class MusicBrainzInfoViewModel(application: Application, private val savedState: SavedStateHandle) : AndroidViewModel(application) {
    private val repository = MusicBrainzRepository(application)
    private val _state = MutableStateFlow<MusicBrainzScreenState>(MusicBrainzScreenState.Loading)
    val state: StateFlow<MusicBrainzScreenState> = _state.asStateFlow()
    private var lookupJob: kotlinx.coroutines.Job? = null

    fun load(kind: String, host: String, expectedTrack: String?) {
        if (savedState.get<String>("kind") == kind && savedState.get<String>("host") == host && _state.value !is MusicBrainzScreenState.Loading) return
        savedState["kind"] = kind
        savedState["host"] = host
        savedState["expected"] = expectedTrack
        retry()
    }

    fun retry() {
        lookupJob?.cancel()
        lookupJob = viewModelScope.launch {
            _state.value = MusicBrainzScreenState.Loading
            try {
                val host = savedState.get<String>("host").orEmpty()
                val expected = savedState.get<String>("expected")
                val metadata = CoverArtFetcher.fetchCurrentSongMetadata(host)
                    ?: throw MusicBrainzNoMatchException()
                if (!matchesExpected(metadata, expected)) {
                    _state.value = MusicBrainzScreenState.Error("The remote track changed before the lookup completed.", false)
                    return@launch
                }
                val kind = savedState.get<String>("kind")
                if (kind == "artist") {
                    if (metadata.artist.isBlank() || metadata.artist.equals("Various Artists", true)) throw MusicBrainzNoMatchException()
                    when (val result = repository.artist(metadata.artist)) {
                        is MusicBrainzLookup.Ready -> _state.value = MusicBrainzScreenState.ArtistReady(result.value, metadata)
                        is MusicBrainzLookup.Choose -> _state.value = MusicBrainzScreenState.Choose(result.candidates)
                    }
                } else {
                    if (metadata.artist.isBlank() || metadata.album.isBlank()) throw MusicBrainzNoMatchException()
                    when (val result = repository.releaseGroup(metadata.artist, metadata.album)) {
                        is MusicBrainzLookup.Ready -> _state.value = MusicBrainzScreenState.AlbumReady(result.value, metadata)
                        is MusicBrainzLookup.Choose -> _state.value = MusicBrainzScreenState.Choose(result.candidates)
                    }
                }
            } catch (_: MusicBrainzNoMatchException) {
                _state.value = MusicBrainzScreenState.Error("No matching information was found on MusicBrainz.", false)
            } catch (e: MusicBrainzNetworkException) {
                _state.value = MusicBrainzScreenState.Error("Unable to retrieve MusicBrainz information. Please check your internet connection.", true)
            } catch (_: Exception) {
                _state.value = MusicBrainzScreenState.Error("Unable to retrieve MusicBrainz information. Please try again.", true)
            }
        }
    }

    fun choose(candidate: MusicBrainzCandidate) {
        lookupJob?.cancel()
        lookupJob = viewModelScope.launch {
            _state.value = MusicBrainzScreenState.Loading
            try {
                val metadata = CoverArtFetcher.fetchCurrentSongMetadata(savedState.get<String>("host").orEmpty())
                    ?: throw MusicBrainzNoMatchException()
                if (!matchesExpected(metadata, savedState.get<String>("expected"))) return@launch
                if (savedState.get<String>("kind") == "artist") {
                    _state.value = MusicBrainzScreenState.ArtistReady(repository.artistById(candidate.id), metadata)
                } else {
                    _state.value = MusicBrainzScreenState.AlbumReady(repository.releaseGroupById(candidate.id), metadata)
                }
            } catch (_: Exception) {
                _state.value = MusicBrainzScreenState.Error("Unable to retrieve MusicBrainz information. Please try again.", true)
            }
        }
    }

    fun cancel() {
        lookupJob?.cancel()
    }

    private fun matchesExpected(metadata: RemoteTrackMetadata, expected: String?): Boolean {
        if (expected.isNullOrBlank()) return false
        fun normalized(value: String) = value.substringAfterLast('/').substringBeforeLast('.').trim().lowercase()
        val current = normalized(expected)
        return current == normalized(metadata.title) || current == normalized(metadata.file) ||
            current.contains(normalized(metadata.title)) || normalized(metadata.title).contains(current)
    }
}
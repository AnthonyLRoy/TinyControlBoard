package com.tinycb.remote.model

data class RemoteTrackMetadata(
    val title: String,
    val artist: String,
    val album: String,
    val file: String,
    val coverUrl: String? = null
) {
    val identity: String
        get() = file.ifBlank { "$artist\u0000$album\u0000$title" }
}
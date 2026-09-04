package com.tinycb.remote.model

data class LibraryEntry(
	val index: Int,
	val total: Int,
	val isDirectory: Boolean,
	val name: String,
	val isRadioStation: Boolean = false
)

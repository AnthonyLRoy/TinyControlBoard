package com.tinycb.remote.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ItemAlbumHeaderBinding
import com.tinycb.remote.databinding.ItemLibraryEntryBinding
import com.tinycb.remote.model.LibraryEntry
import com.tinycb.remote.net.MoodeSettings
import com.tinycb.remote.net.ThumbnailFetcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch

sealed class LibraryRow {
    object Up : LibraryRow()
    data class Entry(val entry: LibraryEntry) : LibraryRow()
    data class AlbumHeader(val albumName: String) : LibraryRow()
}

class LibraryAdapter(
    private val scope: CoroutineScope,
    private val onUpClicked: (() -> Unit)? = null,
    private val onFolderClicked: ((Int) -> Unit)? = null,
    private val onTrackClicked: ((Int) -> Unit)? = null,
    private val onAlbumClicked: ((String) -> Unit)? = null,
    private val playlistMode: Boolean = false,
    private val onDragRequested: ((RecyclerView.ViewHolder) -> Unit)? = null
) : ListAdapter<LibraryRow, RecyclerView.ViewHolder>(DIFF) {

    private var currentTrack: String? = null

    fun setCurrentTrack(track: String?) {
        if (currentTrack == track) return
        val previousTrack = currentTrack
        currentTrack = track
        if (!playlistMode) return

        for (position in 0 until itemCount) {
            val row = getItem(position) as? LibraryRow.Entry ?: continue
            val wasCurrent = isCurrentTrack(row.entry.name, previousTrack)
            val isCurrent = isCurrentTrack(row.entry.name, track)
            if (wasCurrent != isCurrent) notifyItemChanged(position)
        }
    }

    private fun isCurrentTrack(entryName: String, nowPlaying: String?): Boolean {
        if (nowPlaying.isNullOrBlank()) return false
        fun normalize(value: String): String = value.substringAfterLast('/').substringBeforeLast('.')
            .trim().lowercase()
        val entry = normalize(entryName)
        val current = normalize(nowPlaying)
        return entry == current || entry.contains(current) || current.contains(entry)
    }

    override fun getItemViewType(position: Int): Int = when (getItem(position)) {
        is LibraryRow.AlbumHeader -> VIEW_TYPE_ALBUM_HEADER
        else -> VIEW_TYPE_ENTRY
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder =
        if (viewType == VIEW_TYPE_ALBUM_HEADER) {
            AlbumHeaderVH(ItemAlbumHeaderBinding.inflate(LayoutInflater.from(parent.context), parent, false))
        } else {
            EntryVH(ItemLibraryEntryBinding.inflate(LayoutInflater.from(parent.context), parent, false))
        }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        when (holder) {
            is AlbumHeaderVH -> holder.bind(getItem(position) as LibraryRow.AlbumHeader)
            is EntryVH -> holder.bind(getItem(position))
        }
    }

    inner class AlbumHeaderVH(private val b: ItemAlbumHeaderBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(row: LibraryRow.AlbumHeader) {
            b.tvAlbumHeaderName.text = row.albumName
            if (onAlbumClicked != null) {
                b.root.isClickable = true
                b.root.isFocusable = true
                b.root.setOnClickListener { onAlbumClicked.invoke(row.albumName) }
            } else {
                b.root.isClickable = false
                b.root.isFocusable = false
                b.root.setOnClickListener(null)
            }
        }
    }

    inner class EntryVH(private val b: ItemLibraryEntryBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(row: LibraryRow) {
            b.tvTrackNumber.visibility = View.GONE
            b.tvFileType.visibility = View.GONE
            b.tvEntrySubtitle.visibility = View.GONE
            b.ivEntryIcon.visibility = View.VISIBLE
            b.ivAlbumArt.visibility = View.GONE
            b.ivAlbumArt.tag = null
            b.ivDragHandle.visibility = if (playlistMode) View.VISIBLE else View.GONE
            b.ivDragHandle.setOnTouchListener(null)
            b.ivDragHandle.setOnClickListener(null)
            if (playlistMode && onDragRequested != null) {
                b.ivDragHandle.setOnTouchListener { _, event ->
                    if (event.action == android.view.MotionEvent.ACTION_DOWN) {
                        onDragRequested.invoke(this@EntryVH)
                    }
                    true
                }
            }
            b.root.setCardBackgroundColor(b.root.context.getColor(R.color.bg_card))
            
            when (row) {
                is LibraryRow.Up -> {
                    b.tvEntryName.text = b.root.context.getString(R.string.library_up)
                    b.ivEntryIcon.setImageResource(R.drawable.ic_arrow_up)
                    b.ivEntryChevron.visibility = View.GONE
                    b.root.setOnClickListener { onUpClicked?.invoke() }
                }
                is LibraryRow.Entry -> {
                    val entry = row.entry
                    b.tvEntryName.setSingleLine(!playlistMode)
                    b.tvEntryName.maxLines = if (playlistMode) 3 else 1
                    b.tvEntryName.ellipsize = if (playlistMode) android.text.TextUtils.TruncateAt.END else android.text.TextUtils.TruncateAt.END
                    val isCurrent = playlistMode && isCurrentTrack(entry.name, currentTrack)
                    if (isCurrent) {
                        b.root.setCardBackgroundColor(b.root.context.getColor(R.color.bg_card_pressed))
                    }
                    val isRadio = entry.isRadioStation ||
                        entry.name.startsWith("http://", ignoreCase = true) ||
                        entry.name.startsWith("https://", ignoreCase = true) ||
                        entry.name.startsWith("mms://", ignoreCase = true) ||
                        entry.name.startsWith("rtsp://", ignoreCase = true) ||
                        entry.name.endsWith(".pls", ignoreCase = true) ||
                        entry.name.endsWith(".m3u", ignoreCase = true) ||
                        entry.name.endsWith(".m3u8", ignoreCase = true) ||
                        entry.name.endsWith(".asx", ignoreCase = true)

                    if (entry.isDirectory) {
                        b.tvEntryName.text = entry.name
                        b.ivEntryIcon.setImageResource(R.drawable.ic_folder)
                        b.ivEntryChevron.visibility = View.VISIBLE
                        b.root.setOnClickListener { onFolderClicked?.invoke(entry.index) }
                    } else if (isRadio) {
                        var displayName = entry.name
                        if (displayName.endsWith(".pls", ignoreCase = true) ||
                            displayName.endsWith(".m3u", ignoreCase = true) ||
                            displayName.endsWith(".m3u8", ignoreCase = true) ||
                            displayName.endsWith(".asx", ignoreCase = true)) {
                            displayName = displayName.substringBeforeLast('.')
                        }
                        b.tvEntryName.text = displayName
                        b.ivEntryIcon.setImageResource(R.drawable.ic_radio)
                        b.ivEntryChevron.visibility = View.GONE
                        b.root.setOnClickListener { onTrackClicked?.invoke(entry.index) }
                    } else {
                        // Track
                        val (trackNum, cleanName, ext) = parseTrackInfo(entry.name)
                        b.tvEntryName.text = cleanName

                        if (entry.albumName.isNotEmpty()) {
                            b.tvEntrySubtitle.text = entry.albumName
                            b.tvEntrySubtitle.visibility = View.VISIBLE
                        }
                        
                        if (trackNum != null) {
                            b.tvTrackNumber.text = trackNum
                            b.tvTrackNumber.visibility = View.VISIBLE
                        }
                        
                        if (ext != null) {
                            b.tvFileType.text = ext.uppercase()
                            b.tvFileType.visibility = View.VISIBLE
                        }

                        b.ivEntryIcon.visibility = View.GONE
                        b.ivAlbumArt.visibility = View.VISIBLE
                        showAlbumArtPlaceholder()
                        val artHash = entry.albumArtHash
                        if (artHash.isNotEmpty()) {
                            b.ivAlbumArt.tag = artHash
                            scope.launch {
                                val bitmap = ThumbnailFetcher.fetch(
                                    artHash,
                                    MoodeSettings.getHost(b.root.context)
                                )
                                if (b.ivAlbumArt.tag != artHash) return@launch
                                if (bitmap != null) {
                                    b.ivAlbumArt.scaleType = android.widget.ImageView.ScaleType.CENTER_CROP
                                    b.ivAlbumArt.imageTintList = null
                                    b.ivAlbumArt.setImageBitmap(bitmap)
                                }
                            }
                        }
                        b.ivEntryChevron.visibility = View.GONE
                        b.root.setOnClickListener { onTrackClicked?.invoke(entry.index) }
                    }
                }
                is LibraryRow.AlbumHeader -> Unit // handled by AlbumHeaderVH
            }
        }

        private fun showAlbumArtPlaceholder() {
            b.ivAlbumArt.scaleType = android.widget.ImageView.ScaleType.CENTER_INSIDE
            b.ivAlbumArt.imageTintList = android.content.res.ColorStateList.valueOf(
                b.root.context.getColor(R.color.text_secondary)
            )
            b.ivAlbumArt.setImageResource(R.drawable.ic_album)
        }

        private fun parseTrackInfo(fullName: String): Triple<String?, String, String?> {
            var trackNum: String? = null
            var name = fullName
            var ext: String? = null

            // 1. Extract extension
            val lastDot = fullName.lastIndexOf('.')
            if (lastDot != -1 && lastDot > fullName.length - 6) {
                val potentialExt = fullName.substring(lastDot + 1)
                if (potentialExt.length in 2..4) {
                    ext = potentialExt
                    name = fullName.substring(0, lastDot)
                }
            }

            // 2. Extract track number (look for "01 - ", "01. ", or "01 ")
            val trackRegex = Regex("""^(\d{1,3})[\s.\-_]+(.*)$""")
            val match = trackRegex.find(name)
            if (match != null) {
                trackNum = match.groupValues[1].padStart(2, '0')
                name = match.groupValues[2].trim()
            }

            return Triple(trackNum, name.trim(), ext)
        }
    }

    companion object {
        private const val VIEW_TYPE_ENTRY = 0
        private const val VIEW_TYPE_ALBUM_HEADER = 1

        private val DIFF = object : DiffUtil.ItemCallback<LibraryRow>() {
            override fun areItemsTheSame(a: LibraryRow, b: LibraryRow): Boolean {
                if (a is LibraryRow.Up && b is LibraryRow.Up) return true
                if (a is LibraryRow.Entry && b is LibraryRow.Entry) return a.entry.index == b.entry.index
                if (a is LibraryRow.AlbumHeader && b is LibraryRow.AlbumHeader) return a.albumName == b.albumName
                return false
            }
            override fun areContentsTheSame(a: LibraryRow, b: LibraryRow) = a == b
        }
    }
}


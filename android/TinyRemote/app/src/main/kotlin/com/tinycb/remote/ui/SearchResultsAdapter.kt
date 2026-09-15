package com.tinycb.remote.ui

import android.view.LayoutInflater
import android.view.ViewGroup
import android.widget.ImageView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ItemSearchAlbumHeaderBinding
import com.tinycb.remote.databinding.ItemSearchTrackBinding
import com.tinycb.remote.model.LibraryEntry
import com.tinycb.remote.net.ThumbnailFetcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch

sealed class SearchRow {
    data class AlbumHeader(
        val albumName: String,
        val matchCount: Int,
        val albumArtHash: String = "",
        val isExpanded: Boolean = false
    ) : SearchRow()
    data class Track(val entry: LibraryEntry) : SearchRow()
}

/** Flat/grouped track list for [SearchResultsActivity], styled after the redesign mockup:
 * plain divided rows, a note icon, an optional FLAC/MP3 badge derived from the filename
 * extension, and (when grouped) a sticky album header showing a match count. */
class SearchResultsAdapter(
    private val scope: CoroutineScope,
    private val onTrackClicked: (Int) -> Unit,
    private val onAlbumAddClicked: (String) -> Unit,
    private val onAlbumReplaceClicked: (String) -> Unit,
    private val onAlbumExpandClicked: (String) -> Unit
) : ListAdapter<SearchRow, RecyclerView.ViewHolder>(DIFF) {

    override fun getItemViewType(position: Int): Int = when (getItem(position)) {
        is SearchRow.AlbumHeader -> VIEW_TYPE_HEADER
        is SearchRow.Track -> VIEW_TYPE_TRACK
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder =
        if (viewType == VIEW_TYPE_HEADER) {
            HeaderVH(ItemSearchAlbumHeaderBinding.inflate(LayoutInflater.from(parent.context), parent, false))
        } else {
            TrackVH(ItemSearchTrackBinding.inflate(LayoutInflater.from(parent.context), parent, false))
        }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        when (holder) {
            is HeaderVH -> holder.bind(getItem(position) as SearchRow.AlbumHeader)
            is TrackVH -> holder.bind(getItem(position) as SearchRow.Track)
        }
    }

    inner class HeaderVH(private val b: ItemSearchAlbumHeaderBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(row: SearchRow.AlbumHeader) {
            b.tvSearchAlbumName.text = row.albumName
            b.tvSearchAlbumMatches.text = b.root.resources.getQuantityString(
                R.plurals.search_results_match_count, row.matchCount, row.matchCount
            )
            b.root.setOnClickListener { onAlbumExpandClicked(row.albumName) }

            b.btnSearchAlbumAdd.setOnClickListener { onAlbumAddClicked(row.albumName) }
            b.btnSearchAlbumReplace.setOnClickListener { onAlbumReplaceClicked(row.albumName) }
            b.btnSearchAlbumExpand.setOnClickListener { onAlbumExpandClicked(row.albumName) }

            b.btnSearchAlbumExpand.setImageResource(
                if (row.isExpanded) R.drawable.ic_expand_less else R.drawable.ic_expand_more
            )
            b.btnSearchAlbumExpand.contentDescription = b.root.context.getString(
                if (row.isExpanded) R.string.search_results_collapse_album else R.string.search_results_expand_album
            )

            showPlaceholderArt()

            val hash = row.albumArtHash
            if (hash.isNotEmpty()) {
                // Tag guards against this ViewHolder being recycled/rebound to a different row
                // before the async fetch completes.
                b.ivSearchAlbumArt.tag = hash
                scope.launch {
                    val bitmap = ThumbnailFetcher.fetch(hash)
                    if (b.ivSearchAlbumArt.tag != hash) return@launch
                    if (bitmap != null) {
                        b.ivSearchAlbumArt.scaleType = ImageView.ScaleType.CENTER_CROP
                        b.ivSearchAlbumArt.imageTintList = null
                        b.ivSearchAlbumArt.setImageBitmap(bitmap)
                    } else {
                        showPlaceholderArt()
                    }
                }
            }
        }

        private fun showPlaceholderArt() {
            b.ivSearchAlbumArt.tag = null
            b.ivSearchAlbumArt.scaleType = ImageView.ScaleType.CENTER_INSIDE
            b.ivSearchAlbumArt.imageTintList = android.content.res.ColorStateList.valueOf(
                ContextCompat.getColor(b.root.context, R.color.text_secondary)
            )
            b.ivSearchAlbumArt.setImageResource(R.drawable.ic_album)
        }
    }

    inner class TrackVH(private val b: ItemSearchTrackBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(row: SearchRow.Track) {
            val entry = row.entry
            val ext = entry.name.substringAfterLast('.', missingDelimiterValue = "").uppercase()
            b.tvSearchTrackTitle.text = entry.name.substringBeforeLast('.').ifEmpty { entry.name }

            if (ext.isNotEmpty() && ext.length in 2..4) {
                b.tvSearchTrackBadge.visibility = android.view.View.VISIBLE
                b.tvSearchTrackBadge.text = ext
                val context = b.root.context
                if (ext == "FLAC") {
                    b.tvSearchTrackBadge.setBackgroundResource(R.drawable.bg_search_badge_flac)
                    b.tvSearchTrackBadge.setTextColor(ContextCompat.getColor(context, R.color.mcintosh_green))
                } else {
                    b.tvSearchTrackBadge.setBackgroundResource(R.drawable.bg_search_badge_mp3)
                    b.tvSearchTrackBadge.setTextColor(ContextCompat.getColor(context, R.color.search_badge_mp3_text))
                }
            } else {
                b.tvSearchTrackBadge.visibility = android.view.View.GONE
            }

            b.root.setOnClickListener { onTrackClicked(entry.index) }
        }
    }

    companion object {
        private const val VIEW_TYPE_HEADER = 0
        private const val VIEW_TYPE_TRACK = 1

        private val DIFF = object : DiffUtil.ItemCallback<SearchRow>() {
            override fun areItemsTheSame(a: SearchRow, b: SearchRow): Boolean = when {
                a is SearchRow.AlbumHeader && b is SearchRow.AlbumHeader -> a.albumName == b.albumName
                a is SearchRow.Track && b is SearchRow.Track -> a.entry.index == b.entry.index
                else -> false
            }
            override fun areContentsTheSame(a: SearchRow, b: SearchRow) = a == b
        }
    }
}

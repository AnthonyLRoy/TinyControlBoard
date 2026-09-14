package com.tinycb.remote.ui

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ItemSearchAlbumHeaderBinding
import com.tinycb.remote.databinding.ItemSearchTrackBinding
import com.tinycb.remote.model.LibraryEntry

sealed class SearchRow {
    data class AlbumHeader(val albumName: String, val matchCount: Int) : SearchRow()
    data class Track(val entry: LibraryEntry) : SearchRow()
}

/** Flat/grouped track list for [SearchResultsActivity], styled after the redesign mockup:
 * plain divided rows, a note icon, an optional FLAC/MP3 badge derived from the filename
 * extension, and (when grouped) a sticky album header showing a match count. */
class SearchResultsAdapter(
    private val onTrackClicked: (Int) -> Unit,
    private val onAlbumClicked: (String) -> Unit
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
            b.root.setOnClickListener { onAlbumClicked(row.albumName) }
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

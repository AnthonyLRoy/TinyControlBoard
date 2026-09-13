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

sealed class LibraryRow {
    object Up : LibraryRow()
    data class Entry(val entry: LibraryEntry) : LibraryRow()
    data class AlbumHeader(val albumName: String) : LibraryRow()
}

class LibraryAdapter(
    private val onUpClicked: (() -> Unit)? = null,
    private val onFolderClicked: ((Int) -> Unit)? = null,
    private val onTrackClicked: ((Int) -> Unit)? = null,
    private val onAlbumClicked: ((String) -> Unit)? = null
) : ListAdapter<LibraryRow, RecyclerView.ViewHolder>(DIFF) {

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
            when (row) {
                is LibraryRow.Up -> {
                    b.tvEntryName.text = b.root.context.getString(R.string.library_up)
                    b.ivEntryIcon.setImageResource(R.drawable.ic_arrow_up)
                    b.ivEntryChevron.visibility = View.GONE
                    b.root.setOnClickListener { onUpClicked?.invoke() }
                }
                is LibraryRow.Entry -> {
                    val entry = row.entry
                    b.tvEntryName.text = entry.name
                    if (entry.isDirectory) {
                        b.ivEntryIcon.setImageResource(R.drawable.ic_folder)
                        b.ivEntryChevron.visibility = View.VISIBLE
                        b.root.setOnClickListener { onFolderClicked?.invoke(entry.index) }
                    } else {
                        b.ivEntryIcon.setImageResource(
                            if (entry.isRadioStation) R.drawable.ic_radio_wave else R.drawable.ic_music_note
                        )
                        b.ivEntryChevron.visibility = View.GONE
                        b.root.setOnClickListener { onTrackClicked?.invoke(entry.index) }
                    }
                }
                is LibraryRow.AlbumHeader -> Unit // handled by AlbumHeaderVH
            }
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


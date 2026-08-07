package com.tinycb.remote.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ItemLibraryEntryBinding
import com.tinycb.remote.model.LibraryEntry

sealed class LibraryRow {
    object Up : LibraryRow()
    data class Entry(val entry: LibraryEntry) : LibraryRow()
}

class LibraryAdapter(
    private val onUpClicked: () -> Unit,
    private val onFolderClicked: (Int) -> Unit,
    private val onTrackClicked: (Int) -> Unit
) : ListAdapter<LibraryRow, LibraryAdapter.VH>(DIFF) {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH =
        VH(ItemLibraryEntryBinding.inflate(LayoutInflater.from(parent.context), parent, false))

    override fun onBindViewHolder(holder: VH, position: Int) = holder.bind(getItem(position))

    inner class VH(private val b: ItemLibraryEntryBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(row: LibraryRow) {
            when (row) {
                is LibraryRow.Up -> {
                    b.tvEntryName.text = b.root.context.getString(R.string.library_up)
                    b.ivEntryIcon.setImageResource(R.drawable.ic_arrow_up)
                    b.ivEntryChevron.visibility = View.GONE
                    b.root.setOnClickListener { onUpClicked() }
                }
                is LibraryRow.Entry -> {
                    val entry = row.entry
                    b.tvEntryName.text = entry.name
                    if (entry.isDirectory) {
                        b.ivEntryIcon.setImageResource(R.drawable.ic_folder)
                        b.ivEntryChevron.visibility = View.VISIBLE
                        b.root.setOnClickListener { onFolderClicked(entry.index) }
                    } else {
                        b.ivEntryIcon.setImageResource(R.drawable.ic_music_note)
                        b.ivEntryChevron.visibility = View.GONE
                        b.root.setOnClickListener { onTrackClicked(entry.index) }
                    }
                }
            }
        }
    }

    companion object {
        private val DIFF = object : DiffUtil.ItemCallback<LibraryRow>() {
            override fun areItemsTheSame(a: LibraryRow, b: LibraryRow): Boolean {
                if (a is LibraryRow.Up && b is LibraryRow.Up) return true
                if (a is LibraryRow.Entry && b is LibraryRow.Entry) return a.entry.index == b.entry.index
                return false
            }
            override fun areContentsTheSame(a: LibraryRow, b: LibraryRow) = a == b
        }
    }
}

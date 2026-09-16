package com.tinycb.remote.ui

import android.content.Intent
import android.os.Bundle
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.compose.ui.platform.ComposeView
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.ItemTouchHelper
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ActivityPlaylistBinding
import com.tinycb.remote.model.LibraryEntry
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class PlaylistActivity : AppCompatActivity() {

    private lateinit var b: ActivityPlaylistBinding
    private val vm: MainViewModel by viewModels()
    private val adapter = LibraryAdapter(
        onTrackClicked = { index -> vm.library.playTrack(index) },
        playlistMode = true
    )
    private var currentEntries: List<LibraryEntry> = emptyList()
    private var draggedFrom = RecyclerView.NO_POSITION
    private var draggedTo = RecyclerView.NO_POSITION

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityPlaylistBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        b.rvPlaylist.layoutManager = LinearLayoutManager(this)
        b.rvPlaylist.adapter = adapter
        ItemTouchHelper(queueTouchCallback).attachToRecyclerView(b.rvPlaylist)

        b.btnPlaylistManagement.setOnClickListener {
            startActivity(
                Intent(this, PlaylistManagementActivity::class.java)
                    .putExtra(PlaylistManagementActivity.EXTRA_QUEUE_EMPTY, currentEntries.isEmpty())
            )
        }
        b.btnBrowseLibrary.setOnClickListener {
            startActivity(Intent(this, LibraryActivity::class.java))
        }

        lifecycleScope.launch {
            vm.library.listing.collectLatest { entries ->
                currentEntries = entries
                adapter.submitList(entries.map { LibraryRow.Entry(it) })
                b.layoutPlaylistEmpty.visibility = if (entries.isEmpty()) android.view.View.VISIBLE else android.view.View.GONE
            }
        }

        lifecycleScope.launch {
            vm.boardStatus.collectLatest { status ->
                adapter.setCurrentTrack(status?.nowPlaying)
            }
        }
    }

    override fun onResume() {
        super.onResume()
        // Refetch every time this screen becomes visible so Clear/Load done from the
        // playlist-management screen (which doesn't return a result here) show up.
        vm.library.requestQueue()
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            finish()
            return true
        }
        return super.onOptionsItemSelected(item)
    }

    private val queueTouchCallback = object : ItemTouchHelper.SimpleCallback(
        ItemTouchHelper.UP or ItemTouchHelper.DOWN,
        ItemTouchHelper.LEFT
    ) {
        override fun onMove(
            recyclerView: RecyclerView,
            viewHolder: RecyclerView.ViewHolder,
            target: RecyclerView.ViewHolder
        ): Boolean {
            val from = viewHolder.bindingAdapterPosition
            val to = target.bindingAdapterPosition
            if (from == RecyclerView.NO_POSITION || to == RecyclerView.NO_POSITION) return false
            if (draggedFrom == RecyclerView.NO_POSITION) draggedFrom = from
            draggedTo = to
            val reordered = currentEntries.toMutableList().apply { add(to, removeAt(from)) }
            currentEntries = reordered
            adapter.submitList(reordered.map { LibraryRow.Entry(it) })
            return true
        }

        override fun onSwiped(viewHolder: RecyclerView.ViewHolder, direction: Int) {
            val position = viewHolder.bindingAdapterPosition
            if (position == RecyclerView.NO_POSITION || position >= currentEntries.size) return
            val entry = currentEntries[position]
            vm.library.removeTrack(entry.index)
            b.rvPlaylist.postDelayed({ vm.library.requestQueue() }, 300L)
        }

        override fun onSelectedChanged(viewHolder: RecyclerView.ViewHolder?, actionState: Int) {
            super.onSelectedChanged(viewHolder, actionState)
            if (actionState == ItemTouchHelper.ACTION_STATE_DRAG) {
                draggedFrom = viewHolder?.bindingAdapterPosition ?: RecyclerView.NO_POSITION
                draggedTo = draggedFrom
            }
        }

        override fun clearView(recyclerView: RecyclerView, viewHolder: RecyclerView.ViewHolder) {
            super.clearView(recyclerView, viewHolder)
            if (draggedFrom != RecyclerView.NO_POSITION && draggedTo != RecyclerView.NO_POSITION && draggedFrom != draggedTo) {
                vm.library.moveTrack(draggedFrom, draggedTo)
                recyclerView.postDelayed({ vm.library.requestQueue() }, 300L)
            }
            draggedFrom = RecyclerView.NO_POSITION
            draggedTo = RecyclerView.NO_POSITION
        }
    }
}
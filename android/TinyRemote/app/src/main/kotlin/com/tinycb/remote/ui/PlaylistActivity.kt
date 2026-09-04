package com.tinycb.remote.ui

import android.content.Intent
import android.os.Bundle
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.compose.ui.platform.ComposeView
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.google.android.material.dialog.MaterialAlertDialogBuilder
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
        onTrackClicked = { index -> showTrackOptions(index) }
    )
    private var currentEntries: List<LibraryEntry> = emptyList()

    private fun showTrackOptions(index: Int) {
        val entry = currentEntries.firstOrNull { it.index == index } ?: return
        MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
            .setTitle(entry.name)
            .setItems(arrayOf(
                getString(R.string.playlist_play_now),
                getString(R.string.playlist_remove)
            )) { _, which ->
                when (which) {
                    0 -> vm.library.playTrack(index)
                    1 -> {
                        vm.library.removeTrack(index)
                        vm.library.requestQueue()
                    }
                }
            }
            .show()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityPlaylistBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        b.rvPlaylist.layoutManager = LinearLayoutManager(this)
        b.rvPlaylist.adapter = adapter

        b.btnPlaylistManagement.setOnClickListener {
            startActivity(
                Intent(this, PlaylistManagementActivity::class.java)
                    .putExtra(PlaylistManagementActivity.EXTRA_QUEUE_EMPTY, currentEntries.isEmpty())
            )
        }

        lifecycleScope.launch {
            vm.library.listing.collectLatest { entries ->
                currentEntries = entries
                adapter.submitList(entries.map { LibraryRow.Entry(it) })
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
}
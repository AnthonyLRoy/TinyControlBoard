package com.tinycb.remote.ui

import android.os.Bundle
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
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
        AlertDialog.Builder(this)
            .setTitle(entry.name)
            .setItems(arrayOf(
                getString(R.string.playlist_play_now),
                getString(R.string.playlist_remove)
            )) { _, which ->
                when (which) {
                    0 -> vm.playTrack(index)
                    1 -> {
                        vm.removeTrack(index)
                        vm.requestPlaylist()
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

        lifecycleScope.launch {
            vm.libraryListing.collectLatest { entries ->
                currentEntries = entries
                adapter.submitList(entries.map { LibraryRow.Entry(it) })
            }
        }
        vm.requestPlaylist()
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            finish()
            return true
        }
        return super.onOptionsItemSelected(item)
    }
}
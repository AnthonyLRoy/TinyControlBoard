package com.tinycb.remote.ui

import android.os.Bundle
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.tinycb.remote.databinding.ActivityPlaylistBinding
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class PlaylistActivity : AppCompatActivity() {

    private lateinit var b: ActivityPlaylistBinding
    private val vm: MainViewModel by viewModels()
    private val adapter = LibraryAdapter(
        onTrackClicked = { index -> vm.playTrack(index) }
    )

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
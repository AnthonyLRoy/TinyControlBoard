package com.tinycb.remote.ui

import android.os.Bundle
import android.view.MenuItem
import android.view.View
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ActivitySearchResultsBinding
import com.tinycb.remote.model.LibraryEntry
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class SearchResultsActivity : AppCompatActivity() {

    private lateinit var b: ActivitySearchResultsBinding
    private val vm: MainViewModel by viewModels()
    private lateinit var adapter: SearchResultsAdapter
    private lateinit var stickyHeaderDecoration: StickyHeaderItemDecoration
    private var lastResults: List<LibraryEntry> = emptyList()
    private var currentRows: List<SearchRow> = emptyList()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivitySearchResultsBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        val query = intent.getStringExtra(EXTRA_QUERY).orEmpty()
        b.toolbar.subtitle = getString(R.string.search_results_subtitle, query, "\u2026")

        adapter = SearchResultsAdapter(
            scope = lifecycleScope,
            onTrackClicked = { index ->
                vm.search.addResult(index)
                Toast.makeText(this, R.string.search_results_added, Toast.LENGTH_SHORT).show()
            },
            onAlbumClicked = { albumName ->
                showAddAlbumDialog(albumName)
            }
        )

        stickyHeaderDecoration = StickyHeaderItemDecoration { position ->
            currentRows.getOrNull(position) is SearchRow.AlbumHeader
        }

        b.rvSearchResults.layoutManager = LinearLayoutManager(this)
        b.rvSearchResults.adapter = adapter
        b.rvSearchResults.addItemDecoration(stickyHeaderDecoration)

        b.swGroupByAlbum.setOnCheckedChangeListener { _, _ -> renderResults() }

        lifecycleScope.launch {
            vm.search.results.collectLatest { entries ->
                if (entries == null) return@collectLatest
                lastResults = entries
                b.toolbar.subtitle = getString(
                    R.string.search_results_subtitle,
                    query,
                    resources.getQuantityString(R.plurals.search_results_track_count, entries.size, entries.size)
                )
                renderResults()
                b.tvSearchResultsEmpty.visibility = if (entries.isEmpty()) View.VISIBLE else View.GONE
            }
        }
    }

    private fun showAddAlbumDialog(albumName: String) {
        MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
            .setTitle(albumName)
            .setPositiveButton(R.string.search_results_add_album_to_playlist) { _, _ ->
                addAlbumToPlaylist(albumName)
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun addAlbumToPlaylist(albumName: String) {
        val unknownAlbumLabel = getString(R.string.search_results_unknown_album)
        val albumTracks = lastResults.filter { entry ->
            val label = entry.albumName.ifEmpty { unknownAlbumLabel }
            label == albumName
        }
        if (albumTracks.isEmpty()) return

        lifecycleScope.launch {
            for (track in albumTracks) {
                vm.search.addResult(track.index)
                delay(20L)
            }
            Toast.makeText(this@SearchResultsActivity, R.string.search_results_added, Toast.LENGTH_SHORT).show()
        }
    }

    /** Builds the row list from the last search results, either flat (one row per track,
     * original order) or grouped into album-header sections with a per-album match count
     * (server already sorts results by album then track number, so consecutive same-album
     * entries form each section). */
    private fun renderResults() {
        val unknownAlbumLabel = getString(R.string.search_results_unknown_album)
        val rows = if (b.swGroupByAlbum.isChecked) {
            val counts = lastResults.groupingBy { it.albumName.ifEmpty { unknownAlbumLabel } }.eachCount()
            buildList<SearchRow> {
                var currentAlbum: String? = null
                for (entry in lastResults) {
                    val albumLabel = entry.albumName.ifEmpty { unknownAlbumLabel }
                    if (albumLabel != currentAlbum) {
                        add(SearchRow.AlbumHeader(albumLabel, counts.getValue(albumLabel), entry.albumArtHash))
                        currentAlbum = albumLabel
                    }
                    add(SearchRow.Track(entry))
                }
            }
        } else {
            lastResults.map<LibraryEntry, SearchRow> { SearchRow.Track(it) }
        }
        currentRows = rows
        stickyHeaderDecoration.invalidateCache()
        adapter.submitList(rows)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            finish()
            return true
        }
        return super.onOptionsItemSelected(item)
    }

    companion object {
        const val EXTRA_QUERY = "extra_query"
    }
}


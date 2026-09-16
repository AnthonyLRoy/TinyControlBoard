package com.tinycb.remote.ui

import android.app.Activity
import android.os.Bundle
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.tinycb.remote.R
import com.tinycb.remote.ble.BleProtocol
import com.tinycb.remote.databinding.ActivityPlaylistListBinding
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.TimeoutCancellationException
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import kotlinx.coroutines.withTimeout

/** Shows saved playlist names for either Load or Delete, sharing the fetch/empty/error states. */
class PlaylistListActivity : AppCompatActivity() {

    private lateinit var b: ActivityPlaylistListBinding
    private val vm: MainViewModel by viewModels()
    private lateinit var adapter: LibraryAdapter
    private var names: List<String> = emptyList()

    private val mode by lazy { intent.getStringExtra(EXTRA_MODE) ?: MODE_LOAD }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityPlaylistListBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.title = getString(
            if (mode == MODE_LOAD) R.string.playlist_load else R.string.playlist_delete
        )

        adapter = LibraryAdapter(
            scope = lifecycleScope,
            onTrackClicked = { index -> onItemTapped(index) }
        )
        b.rvPlaylistNames.layoutManager = LinearLayoutManager(this)
        b.rvPlaylistNames.adapter = adapter
        b.btnEmptyStateBack.setOnClickListener { finish() }

        lifecycleScope.launch {
            vm.playlist.nameEntries.collectLatest { entries ->
                if (entries == null) {
                    b.tvEmptyState.visibility = android.view.View.GONE
                    b.btnEmptyStateBack.visibility = android.view.View.GONE
                    b.rvPlaylistNames.visibility = android.view.View.VISIBLE
                    adapter.submitList(emptyList())
                    return@collectLatest
                }
                names = entries.map { it.name }
                if (entries.isEmpty()) {
                    b.rvPlaylistNames.visibility = android.view.View.GONE
                    b.tvEmptyState.visibility = android.view.View.VISIBLE
                    b.btnEmptyStateBack.visibility = android.view.View.VISIBLE
                    b.tvEmptyState.text = getString(R.string.playlist_none_found)
                } else {
                    b.tvEmptyState.visibility = android.view.View.GONE
                    b.btnEmptyStateBack.visibility = android.view.View.GONE
                    b.rvPlaylistNames.visibility = android.view.View.VISIBLE
                    adapter.submitList(entries.map { LibraryRow.Entry(it) })
                }
            }
        }

        fetchNames()
    }

    private fun fetchNames() {
        vm.playlist.requestNames()
        lifecycleScope.launch {
            try {
                withTimeout(5000) { vm.playlist.nameEntries.first { it != null } }
            } catch (e: TimeoutCancellationException) {
                showFetchFailedDialog()
            }
        }
    }

    private fun showFetchFailedDialog() {
        MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
            .setMessage(R.string.playlist_fetch_failed)
            .setPositiveButton(R.string.retry) { _, _ -> fetchNames() }
            .setNegativeButton(R.string.cancel) { _, _ -> finish() }
            .setCancelable(false)
            .show()
    }

    private fun onItemTapped(index: Int) {
        val name = names.getOrNull(index) ?: return
        if (mode == MODE_LOAD) loadPlaylist(name) else confirmDelete(name)
    }

    private fun loadPlaylist(name: String) {
        vm.playlist.load(name)
        awaitPlaylistOpResult(
            onResult = { result ->
                if (result.ok) {
                    MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
                        .setMessage(R.string.playlist_loaded)
                        .setPositiveButton(R.string.ok) { _, _ ->
                            setResult(Activity.RESULT_OK)
                            finish()
                        }
                        .setCancelable(false)
                        .show()
                } else {
                    showRetryCancelDialog(getString(R.string.playlist_load_failed, result.message)) { loadPlaylist(name) }
                }
            },
            onFailure = { showRetryCancelDialog(getString(R.string.playlist_load_failed, "")) { loadPlaylist(name) } }
        )
    }

    private fun confirmDelete(name: String) {
        MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
            .setMessage(getString(R.string.playlist_delete_confirm, name))
            .setPositiveButton(R.string.playlist_delete_confirm_button) { _, _ -> deletePlaylist(name) }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun deletePlaylist(name: String) {
        vm.playlist.delete(name)
        awaitPlaylistOpResult(
            onResult = { result ->
                if (result.ok) {
                    fetchNames() // refresh the list in place, not the Playlist screen
                } else {
                    showRetryCancelDialog(getString(R.string.playlist_delete_failed, result.message)) { deletePlaylist(name) }
                }
            },
            onFailure = { showRetryCancelDialog(getString(R.string.playlist_delete_failed, "")) { deletePlaylist(name) } }
        )
    }

    private fun awaitPlaylistOpResult(onResult: (BleProtocol.PlaylistOpResult) -> Unit, onFailure: () -> Unit) {
        lifecycleScope.launch {
            try {
                val result = withTimeout(8000) { vm.playlist.opResult.filterNotNull().first() }
                vm.playlist.clearOpResult()
                onResult(result)
            } catch (e: TimeoutCancellationException) {
                onFailure()
            }
        }
    }

    private fun showRetryCancelDialog(message: String, onRetry: () -> Unit) {
        MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
            .setMessage(message)
            .setPositiveButton(R.string.retry) { _, _ -> onRetry() }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            finish()
            return true
        }
        return super.onOptionsItemSelected(item)
    }

    companion object {
        const val EXTRA_MODE = "mode"
        const val MODE_LOAD = "load"
        const val MODE_DELETE = "delete"
    }
}

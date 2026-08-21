package com.tinycb.remote.ui

import android.os.Bundle
import android.view.MenuItem
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ActivityLibraryBinding
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class LibraryActivity : AppCompatActivity() {

    private lateinit var b: ActivityLibraryBinding
    private val vm: MainViewModel by viewModels()
    private lateinit var adapter: LibraryAdapter

    // Local-only folder depth — the RPi holds the real path; we just track whether
    // an "Up" row should be shown (it is never sent to the board as a real index).
    private var depth = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityLibraryBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        adapter = LibraryAdapter(
            onUpClicked = {
                depth = (depth - 1).coerceAtLeast(0)
                vm.browseUp()
            },
            onFolderClicked = { index ->
                showFolderActions(index)
            },
            onTrackClicked = { index ->
                vm.addTrack(index)
                Toast.makeText(this, R.string.library_added_to_playlist, Toast.LENGTH_SHORT).show()
            }
        )

        b.rvLibrary.layoutManager = LinearLayoutManager(this)
        b.rvLibrary.adapter = adapter

        lifecycleScope.launch {
            vm.libraryListing.collectLatest { entries ->
                val rows = buildList {
                    if (depth > 0) add(LibraryRow.Up)
                    entries.forEach { add(LibraryRow.Entry(it)) }
                }
                adapter.submitList(rows)
            }
        }

        vm.browseRoot()
    }

    private fun showFolderActions(index: Int) {
        val options = arrayOf(
            getString(R.string.library_add_folder),
            getString(R.string.library_replace_with_folder),
            getString(R.string.library_open_folder)
        )
        AlertDialog.Builder(this)
            .setTitle(R.string.library_folder_actions_title)
            .setItems(options) { _, which ->
                when (which) {
                    0 -> {
                        vm.addFolder(index)
                        Toast.makeText(this, R.string.library_folder_added, Toast.LENGTH_SHORT).show()
                    }
                    1 -> {
                        vm.replaceWithFolder(index)
                        Toast.makeText(this, R.string.library_playlist_replaced, Toast.LENGTH_SHORT).show()
                    }
                    2 -> {
                        depth += 1
                        vm.browseInto(index)
                    }
                }
            }
            .show()
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            finish()
            return true
        }
        return super.onOptionsItemSelected(item)
    }
}

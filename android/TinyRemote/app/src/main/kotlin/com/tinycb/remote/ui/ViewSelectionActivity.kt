package com.tinycb.remote.ui

import android.os.Bundle
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ActivityViewSelectionBinding
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class ViewSelectionActivity : AppCompatActivity() {

    private lateinit var b: ActivityViewSelectionBinding
    private val vm: MainViewModel by viewModels()
    private lateinit var adapter: ViewSelectionAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityViewSelectionBinding.inflate(layoutInflater)
        setContentView(b.root)

        b.composeBackground.setContent {
            BrushedAluminumSurface(shade = MetallicShade.Titanium)
        }

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        adapter = ViewSelectionAdapter { option ->
            vm.setSelectedView(option.commandId)
        }

        b.rvViews.layoutManager = LinearLayoutManager(this)
        b.rvViews.adapter = adapter

        val options = listOf(
            ViewOption(getString(R.string.view_default),  getString(R.string.view_default_desc),  R.drawable.ic_grid_view,   0x0122),
            ViewOption(getString(R.string.view_radio),    getString(R.string.view_radio_desc),    R.drawable.ic_radio,       0x0123),
            ViewOption(getString(R.string.view_playlist), getString(R.string.view_playlist_desc), R.drawable.ic_queue_music, 0x0124),
            ViewOption(getString(R.string.view_folder),   getString(R.string.view_folder_desc),   R.drawable.ic_folder,      0x0125),
            ViewOption(getString(R.string.view_tag),      getString(R.string.view_tag_desc),      R.drawable.ic_label,       0x0126),
            ViewOption(getString(R.string.view_album),    getString(R.string.view_album_desc),    R.drawable.ic_album,       0x0127)
        )
        adapter.submitList(options)

        lifecycleScope.launch {
            vm.selectedViewId.collectLatest { id ->
                adapter.setSelected(id)
            }
        }
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == android.R.id.home) {
            finish()
            return true
        }
        return super.onOptionsItemSelected(item)
    }
}

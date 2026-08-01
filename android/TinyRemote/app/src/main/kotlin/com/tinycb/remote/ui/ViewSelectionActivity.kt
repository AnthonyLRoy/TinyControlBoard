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

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        adapter = ViewSelectionAdapter { option ->
            vm.setSelectedView(option.commandId)
        }

        b.rvViews.layoutManager = LinearLayoutManager(this)
        b.rvViews.adapter = adapter

        val options = listOf(
            ViewOption(getString(R.string.view_default),  0x0122),
            ViewOption(getString(R.string.view_radio),    0x0123),
            ViewOption(getString(R.string.view_playlist), 0x0124),
            ViewOption(getString(R.string.view_folder),   0x0125),
            ViewOption(getString(R.string.view_tag),      0x0126),
            ViewOption(getString(R.string.view_album),    0x0127)
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

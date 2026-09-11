package com.tinycb.remote.ui

import android.os.Bundle
import android.view.MenuItem
import android.view.View
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ActivitySearchResultsBinding
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class SearchResultsActivity : AppCompatActivity() {

    private lateinit var b: ActivitySearchResultsBinding
    private val vm: MainViewModel by viewModels()
    private lateinit var adapter: LibraryAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivitySearchResultsBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        adapter = LibraryAdapter(
            onTrackClicked = { index ->
                vm.search.addResult(index)
                Toast.makeText(this, R.string.search_results_added, Toast.LENGTH_SHORT).show()
            }
        )

        b.rvSearchResults.layoutManager = LinearLayoutManager(this)
        b.rvSearchResults.adapter = adapter

        lifecycleScope.launch {
            vm.search.results.collectLatest { entries ->
                if (entries == null) return@collectLatest
                adapter.submitList(entries.map { LibraryRow.Entry(it) })
                b.tvSearchResultsEmpty.visibility = if (entries.isEmpty()) View.VISIBLE else View.GONE
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

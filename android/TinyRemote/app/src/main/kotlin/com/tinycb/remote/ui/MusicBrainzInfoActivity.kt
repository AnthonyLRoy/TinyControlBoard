package com.tinycb.remote.ui

import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.widget.*
import androidx.activity.viewModels
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import com.tinycb.remote.R
import com.tinycb.remote.musicbrainz.*
import com.tinycb.remote.net.ArtistImageFetcher
import com.tinycb.remote.net.CoverArtFetcher
import com.google.android.material.chip.Chip
import com.google.android.material.chip.ChipGroup
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class MusicBrainzInfoActivity : AppCompatActivity() {
    private val vm: MusicBrainzInfoViewModel by viewModels()
    private lateinit var content: LinearLayout
    private lateinit var progress: ProgressBar
    private var shownCandidates: String? = null
    private var art: Bitmap? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        buildUi()
        val kind = intent.getStringExtra(EXTRA_KIND).orEmpty()
        val host = intent.getStringExtra(EXTRA_HOST).orEmpty()
        val expected = intent.getStringExtra(EXTRA_EXPECTED_TRACK)
        val testArtist = intent.getStringExtra(EXTRA_TEST_ARTIST).orEmpty()
        lifecycleScope.launch {
            if (testArtist.isBlank()) art = CoverArtFetcher.fetchCoverArt(host)
            vm.load(kind, host, expected, testArtist)
        }
        lifecycleScope.launch { vm.state.collectLatest(::render) }
    }

    private fun buildUi() {
        val root = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; setBackgroundColor(ContextCompat.getColor(context, R.color.bg_primary)) }
        val header = LinearLayout(this).apply { gravity = Gravity.CENTER_VERTICAL; setPadding(16, 8, 8, 8) }
        val title = TextView(this).apply { text = "MusicBrainz"; textSize = 20f; setTextColor(ContextCompat.getColor(context, R.color.text_primary)); layoutParams = LinearLayout.LayoutParams(0, 56, 1f) }
        val close = ImageButton(this).apply { setImageResource(android.R.drawable.ic_menu_close_clear_cancel); contentDescription = "Close"; setBackgroundResource(android.R.color.transparent); setOnClickListener { finish() } }
        header.addView(title); header.addView(close, LinearLayout.LayoutParams(48, 48)); root.addView(header)
        val scroll = ScrollView(this)
        content = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; setPadding(16, 8, 16, 24) }
        scroll.addView(content); root.addView(scroll, LinearLayout.LayoutParams(-1, 0, 1f)); setContentView(root)
        progress = ProgressBar(this).apply { visibility = View.GONE }
    }

    private fun render(state: MusicBrainzScreenState) {
        content.removeAllViews()
        when (state) {
            MusicBrainzScreenState.Loading -> { content.addView(progress); progress.visibility = View.VISIBLE; content.addView(label("MusicBrainz lookup may take a few seconds.")) }
            is MusicBrainzScreenState.Error -> { content.addView(label(state.message)); if (state.retryable) content.addView(button("Retry") { vm.retry() }) }
            is MusicBrainzScreenState.Choose -> showChoices(state.candidates)
            is MusicBrainzScreenState.ArtistReady -> renderArtist(state.value, state.metadata)
            is MusicBrainzScreenState.AlbumReady -> renderAlbum(state.value, state.metadata)
        }
    }

    private fun renderArtist(value: MusicBrainzArtist, metadata: com.tinycb.remote.model.RemoteTrackMetadata) {
        val imageSlot = FrameLayout(this).apply {
            layoutParams = LinearLayout.LayoutParams(-1, 230).apply { bottomMargin = 12 }
        }
        content.addView(imageSlot)
        content.addView(heading(value.name))
        lifecycleScope.launch {
            ArtistImageFetcher.fetch(this@MusicBrainzInfoActivity, value.name)?.let { image ->
                imageSlot.removeAllViews()
                imageSlot.addView(ImageView(this@MusicBrainzInfoActivity).apply {
                    setImageBitmap(image)
                    scaleType = ImageView.ScaleType.CENTER_CROP
                }, FrameLayout.LayoutParams(-1, -1))
                imageSlot.addView(label("Artist image via Wikipedia").apply {
                    setTextColor(ContextCompat.getColor(context, android.R.color.white))
                    setBackgroundColor(0x99000000.toInt())
                    setPadding(8, 4, 8, 4)
                    layoutParams = FrameLayout.LayoutParams(-1, -2, Gravity.BOTTOM)
                })
            }
        }
        section("Profile")
        field("Type", value.type); field("Country", value.country); field("Active", value.lifeSpan); field("Disambiguation", value.disambiguation)
        chipSection("Genres", value.genres); chipSection("Tags", value.tags); chipSection("Aliases", value.aliases)
        links(value.links, "https://musicbrainz.org/artist/${value.id}"); attribution()
    }

    private fun renderAlbum(value: MusicBrainzReleaseGroup, metadata: com.tinycb.remote.model.RemoteTrackMetadata) {
        addArt(); content.addView(heading(value.title)); section("Album details")
        field("Artist", value.artist.ifBlank { metadata.artist }); field("Release date", value.firstReleaseDate); field("Type", listOf(value.primaryType, *value.secondaryTypes.toTypedArray()).filter { it.isNotBlank() }.joinToString(", ")); field("Disambiguation", value.disambiguation)
        chipSection("Genres", value.genres); chipSection("Tags", value.tags)
        value.release?.let { release ->
            section("Release")
            field("Details", listOf(release.date, release.country, release.label).filter { it.isNotBlank() }.joinToString(" · "))
            section("Tracks")
            release.tracks.forEach { field(it.position, it.title) }
        }
        links(value.links, "https://musicbrainz.org/release-group/${value.id}"); attribution()
    }

    private fun addArt() { art?.let { image -> content.addView(ImageView(this).apply { setImageBitmap(image); scaleType = ImageView.ScaleType.CENTER_CROP }, LinearLayout.LayoutParams(-1, 220).apply { bottomMargin = 12 }) } }
    private fun heading(text: String) = TextView(this).apply { this.text = text; textSize = 24f; setTextColor(ContextCompat.getColor(context, R.color.text_primary)); setPadding(0, 8, 0, 12) }
    private fun section(text: String) { content.addView(TextView(this).apply { this.text = text.uppercase(); textSize = 12f; setTextColor(ContextCompat.getColor(context, R.color.text_accent)); setPadding(0, 16, 0, 4) }) }
    private fun label(text: String) = TextView(this).apply { this.text = text; textSize = 16f; setTextColor(ContextCompat.getColor(context, R.color.text_primary)); setPadding(0, 12, 0, 12) }
    private fun field(name: String, value: String) {
        if (value.isNotBlank()) content.addView(LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(0, 4, 0, 4)
            addView(TextView(context).apply { text = name; textSize = 12f; setTextColor(ContextCompat.getColor(context, R.color.text_secondary)) })
            addView(TextView(context).apply { text = value; textSize = 16f; setTextColor(ContextCompat.getColor(context, R.color.text_primary)) })
        })
    }
    private fun chipSection(name: String, values: List<String>) {
        val visible = values.filter { it.isNotBlank() }.distinct()
        if (visible.isEmpty()) return
        section(name)
        content.addView(ChipGroup(this).apply {
            isSingleLine = false
            visible.forEach { value ->
                addView(Chip(context).apply {
                    text = value
                    isClickable = false
                    isCheckable = false
                })
            }
        })
    }
    private fun button(text: String, action: () -> Unit) = Button(this).apply { this.text = text; setOnClickListener { action() } }
    private fun links(values: List<Pair<String, String>>, mbUrl: String) { content.addView(button("Open MusicBrainz page") { startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(mbUrl))) }); values.forEach { (name, url) -> content.addView(button(name) { startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(url))) }) } }
    private fun attribution() { content.addView(label("Data provided by MusicBrainz")) }
    private fun showChoices(candidates: List<MusicBrainzCandidate>) {
        val key = candidates.joinToString { it.id }; if (key == shownCandidates) return; shownCandidates = key
        AlertDialog.Builder(this).setTitle("Choose a MusicBrainz match").setItems(candidates.map { listOf(it.name, it.country, it.type, it.disambiguation).filter(String::isNotBlank).joinToString(" · ") }.toTypedArray()) { _, which -> vm.choose(candidates[which]) }.setNegativeButton("Cancel", null).show()
    }

    override fun onDestroy() { if (isFinishing) vm.cancel(); super.onDestroy() }
    override fun onStop() { if (!isChangingConfigurations && !isFinishing) vm.cancel(); super.onStop() }

    companion object {
        const val EXTRA_KIND = "kind"
        const val EXTRA_HOST = "host"
        const val EXTRA_EXPECTED_TRACK = "expected_track"
        const val EXTRA_TEST_ARTIST = "test_artist"
    }
}
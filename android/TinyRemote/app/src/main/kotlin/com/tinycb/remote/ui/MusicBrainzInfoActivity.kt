package com.tinycb.remote.ui

import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.widget.*
import android.graphics.drawable.GradientDrawable
import android.text.TextUtils
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
import java.text.SimpleDateFormat
import java.util.Locale

class MusicBrainzInfoActivity : AppCompatActivity() {
    private val vm: MusicBrainzInfoViewModel by viewModels()
    private lateinit var content: LinearLayout
    private lateinit var progress: ProgressBar
    private var shownCandidates: String? = null
    private var art: Bitmap? = null
    private var bioText: TextView? = null
    private var bioExpanded = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        buildUi()
        val kind = intent.getStringExtra(EXTRA_KIND).orEmpty()
        val host = intent.getStringExtra(EXTRA_HOST).orEmpty()
        val expected = intent.getStringExtra(EXTRA_EXPECTED_TRACK)
        val testArtist = intent.getStringExtra(EXTRA_TEST_ARTIST).orEmpty()
        val testAlbum = intent.getStringExtra(EXTRA_TEST_ALBUM).orEmpty()
        lifecycleScope.launch {
            if (testArtist.isBlank()) art = CoverArtFetcher.fetchCoverArt(host)
            vm.load(kind, host, expected, testArtist, testAlbum)
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
        val imageSlot = artistHeader(value)
        content.addView(imageSlot)
        section("About")
        val about = TextView(this).apply {
            text = "Biography unavailable."
            textSize = 16f
            setTextColor(ContextCompat.getColor(context, R.color.text_primary))
            setPadding(0, 4, 0, 0)
        }
        bioText = about
        content.addView(about)
        var fullBio = ""
        val showMore = Button(this).apply {
            text = "Show more"
            setOnClickListener {
                bioExpanded = !bioExpanded
                about.text = if (bioExpanded) fullBio else firstParagraphs(fullBio)
                about.maxLines = if (bioExpanded) Int.MAX_VALUE else 8
                about.ellipsize = if (bioExpanded) null else TextUtils.TruncateAt.END
                text = if (bioExpanded) "Show less" else "Show more"
            }
        }
        showMore.visibility = View.GONE
        content.addView(showMore)
        val soundsLike = (value.genres + value.tags).filter { it.isNotBlank() }.distinct()
        expandableChips("Sounds like", soundsLike, 5)
        statCards(value)
        expandableChips("Aliases", value.aliases, 2)
        section("Details")
        field("Disambiguation", value.disambiguation)
        links(value.links, "https://musicbrainz.org/artist/${value.id}"); attribution()
        lifecycleScope.launch {
            val supplement = ArtistImageFetcher.fetchSupplement(this@MusicBrainzInfoActivity, value.name)
            supplement.image?.let { image -> setArtistHeaderImage(imageSlot, image, value) }
            supplement.extract?.let { extract ->
                fullBio = extract
                about.text = firstParagraphs(fullBio)
                about.maxLines = Int.MAX_VALUE
                about.ellipsize = null
                about.post {
                    val needsMore = about.lineCount > 8
                    if (!bioExpanded) {
                        about.maxLines = 8
                        about.ellipsize = TextUtils.TruncateAt.END
                    }
                    showMore.visibility = if (needsMore) View.VISIBLE else View.GONE
                }
            }
        }
    }

    private fun artistHeader(value: MusicBrainzArtist): FrameLayout = FrameLayout(this).apply {
        layoutParams = LinearLayout.LayoutParams(-1, 600).apply {
            marginStart = -16
            marginEnd = -16
            bottomMargin = 16
        }
        setBackgroundColor(ContextCompat.getColor(context, R.color.bg_card))
        addView(TextView(context).apply { text = value.name.takeInitials(); textSize = 56f; gravity = Gravity.CENTER; setTextColor(ContextCompat.getColor(context, R.color.text_secondary)) }, FrameLayout.LayoutParams(-1, -1))
        addView(View(context).apply { background = GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, intArrayOf(0x00000000, 0xBF000000.toInt())) }, FrameLayout.LayoutParams(-1, -1))
        addView(LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(16, 8, 16, 12)
            addView(TextView(context).apply { text = value.name; textSize = 26f; setTextColor(ContextCompat.getColor(context, android.R.color.white)); setTypeface(typeface, android.graphics.Typeface.BOLD) })
            if (value.disambiguation.isNotBlank()) addView(TextView(context).apply { text = value.disambiguation; textSize = 13f; setTextColor(0xDDFFFFFF.toInt()) })
            addView(TextView(context).apply { text = "Artist image via Wikipedia"; textSize = 9f; setTextColor(0xAAFFFFFF.toInt()) })
        }, FrameLayout.LayoutParams(-1, -2, Gravity.BOTTOM))
    }

    private fun setArtistHeaderImage(slot: FrameLayout, image: Bitmap, value: MusicBrainzArtist) {
        val imageView = ImageView(this).apply {
            setImageBitmap(image)
            scaleType = ImageView.ScaleType.FIT_CENTER
            setBackgroundColor(ContextCompat.getColor(context, R.color.bg_card))
        }
        slot.addView(imageView, 0, FrameLayout.LayoutParams(-1, -1))
    }

    private fun statCards(value: MusicBrainzArtist) {
        val row = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; weightSum = 2f; setPadding(0, 4, 0, 4) }
        statCard("Active", formatLifeSpan(value.lifeSpan)).also { row.addView(it, LinearLayout.LayoutParams(0, 72, 1f).apply { marginEnd = 6 }) }
        statCard("Origin", countryName(value.country)).also { row.addView(it, LinearLayout.LayoutParams(0, 72, 1f).apply { marginStart = 6 }) }
        content.addView(row)
    }

    private fun statCard(title: String, value: String) = TextView(this).apply { text = "$title\n$value"; textSize = 14f; setTextColor(ContextCompat.getColor(context, R.color.text_primary)); gravity = Gravity.CENTER_VERTICAL; setPadding(14, 8, 14, 8); background = GradientDrawable().apply { cornerRadius = 12f; setColor(ContextCompat.getColor(context, R.color.bg_card)) } }

    private fun formatLifeSpan(value: String): String = value.replace(" - ", "–").ifBlank { "Unknown" }
    private fun countryName(value: String): String = mapOf("US" to "United States", "GB" to "United Kingdom", "AU" to "Australia", "CA" to "Canada", "DE" to "Germany", "FR" to "France", "IE" to "Ireland", "JP" to "Japan")[value] ?: value.ifBlank { "Unknown" }
    private fun firstParagraphs(value: String): String = value.split(Regex("\\n\\s*\\n")).filter { it.isNotBlank() }.take(2).joinToString("\n\n")
    private fun String.takeInitials(): String = trim().split(Regex("\\s+")).filter { it.isNotEmpty() }.take(2).joinToString("") { it.first().uppercase() }
    private fun formatDate(value: String): String = try {
        SimpleDateFormat("yyyy-MM-dd", Locale.US).parse(value)?.let { SimpleDateFormat("MMMM d, yyyy", Locale.US).format(it) } ?: value
    } catch (_: Exception) { value }

    private fun trackRow(position: String, title: String) {
        content.addView(LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(0, 8, 0, 8)
            addView(TextView(context).apply { text = position; textSize = 13f; setTextColor(ContextCompat.getColor(context, R.color.text_secondary)); gravity = Gravity.CENTER }, LinearLayout.LayoutParams(42, -2))
            addView(TextView(context).apply { text = title; textSize = 16f; setTextColor(ContextCompat.getColor(context, R.color.text_primary)) })
        })
    }

    private fun expandableChips(title: String, values: List<String>, initialCount: Int) {
        val visible = values.filter { it.isNotBlank() }.distinct()
        if (visible.isEmpty()) return
        section(title)
        val group = ChipGroup(this).apply { isSingleLine = false }
        fun render(expanded: Boolean) {
            group.removeAllViews()
            val shown = if (expanded) visible else visible.take(initialCount)
            shown.forEach { value -> group.addView(Chip(this@MusicBrainzInfoActivity).apply { text = value; isClickable = false; isCheckable = false }) }
            if (visible.size > initialCount) group.addView(Chip(this@MusicBrainzInfoActivity).apply { text = if (expanded) "Show less" else "+${visible.size - initialCount} more"; setOnClickListener { render(!expanded) } })
        }
        render(false); content.addView(group)
    }

    private fun renderAlbum(value: MusicBrainzReleaseGroup, metadata: com.tinycb.remote.model.RemoteTrackMetadata) {
        val artistName = value.artist.ifBlank { metadata.artist }
        content.addView(albumHeader(value.title, artistName))
        section("About")
        val about = TextView(this).apply {
            text = "Album description unavailable."
            textSize = 16f
            setTextColor(ContextCompat.getColor(context, R.color.text_primary))
            setPadding(0, 4, 0, 0)
        }
        content.addView(about)
        var fullDescription = ""
        val showMore = Button(this).apply {
            text = "Show more"
            visibility = View.GONE
            setOnClickListener {
                bioExpanded = !bioExpanded
                about.text = if (bioExpanded) fullDescription else firstParagraphs(fullDescription)
                about.maxLines = if (bioExpanded) Int.MAX_VALUE else 8
                about.ellipsize = if (bioExpanded) null else TextUtils.TruncateAt.END
                text = if (bioExpanded) "Show less" else "Show more"
            }
        }
        content.addView(showMore)
        lifecycleScope.launch {
            ArtistImageFetcher.fetchSupplement(this@MusicBrainzInfoActivity, value.title).extract?.let { extract ->
                fullDescription = extract
                about.text = firstParagraphs(fullDescription)
                about.maxLines = Int.MAX_VALUE
                about.ellipsize = null
                about.post {
                    val needsMore = about.lineCount > 8
                    if (!bioExpanded) {
                        about.maxLines = 8
                        about.ellipsize = TextUtils.TruncateAt.END
                    }
                    showMore.visibility = if (needsMore) View.VISIBLE else View.GONE
                }
            }
        }
        section("Album details")
        field("Artist", artistName); field("Release date", formatDate(value.firstReleaseDate)); field("Type", listOf(value.primaryType, *value.secondaryTypes.toTypedArray()).filter { it.isNotBlank() }.joinToString(", ")); field("Disambiguation", value.disambiguation)
        expandableChips("Sounds like", (value.genres + value.tags).filter { it.isNotBlank() }.distinct(), 5)
        value.release?.let { release ->
            section("Release")
            field("Details", listOf(formatDate(release.date), countryName(release.country), release.label).filter { it.isNotBlank() }.joinToString(" · "))
            section("Tracks")
            release.tracks.forEach { trackRow(it.position, it.title) }
        }
        links(value.links, "https://musicbrainz.org/release-group/${value.id}"); attribution()
    }

    private fun albumHeader(title: String, artist: String): FrameLayout = FrameLayout(this).apply {
        layoutParams = LinearLayout.LayoutParams(-1, 600).apply {
            marginStart = -16
            marginEnd = -16
            bottomMargin = 16
        }
        setBackgroundColor(ContextCompat.getColor(context, R.color.bg_card))
        art?.let { image -> addView(ImageView(context).apply { setImageBitmap(image); scaleType = ImageView.ScaleType.FIT_CENTER; setBackgroundColor(ContextCompat.getColor(context, R.color.bg_card)) }, FrameLayout.LayoutParams(-1, -1)) }
            ?: addView(TextView(context).apply { text = title.takeInitials(); textSize = 56f; gravity = Gravity.CENTER; setTextColor(ContextCompat.getColor(context, R.color.text_secondary)) }, FrameLayout.LayoutParams(-1, -1))
        addView(View(context).apply { background = GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, intArrayOf(0x00000000, 0xBF000000.toInt())) }, FrameLayout.LayoutParams(-1, -1))
        addView(LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(16, 8, 16, 12)
            addView(TextView(context).apply { text = title; textSize = 26f; setTextColor(ContextCompat.getColor(context, android.R.color.white)); setTypeface(typeface, android.graphics.Typeface.BOLD) })
            if (artist.isNotBlank()) addView(TextView(context).apply { text = artist; textSize = 15f; setTextColor(0xDDFFFFFF.toInt()) })
        }, FrameLayout.LayoutParams(-1, -2, Gravity.BOTTOM))
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
        const val EXTRA_TEST_ALBUM = "test_album"
    }
}
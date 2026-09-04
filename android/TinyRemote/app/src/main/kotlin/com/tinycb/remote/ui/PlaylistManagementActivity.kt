package com.tinycb.remote.ui

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.MenuItem
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.tinycb.remote.R
import com.tinycb.remote.ble.BleProtocol
import com.tinycb.remote.databinding.ActivityPlaylistManagementBinding
import com.tinycb.remote.databinding.DialogSavePlaylistBinding
import com.tinycb.remote.viewmodel.MainViewModel
import kotlinx.coroutines.TimeoutCancellationException
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import kotlinx.coroutines.withTimeout

class PlaylistManagementActivity : AppCompatActivity() {

    private lateinit var b: ActivityPlaylistManagementBinding
    private val vm: MainViewModel by viewModels()

    private val playlistListLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == Activity.RESULT_OK) finish()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityPlaylistManagementBinding.inflate(layoutInflater)
        setContentView(b.root)

        setSupportActionBar(b.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        val queueEmpty = intent.getBooleanExtra(EXTRA_QUEUE_EMPTY, false)

        b.btnSavePlaylist.setOnClickListener { showSaveDialog(queueEmpty) }
        b.btnLoadPlaylist.setOnClickListener {
            playlistListLauncher.launch(
                Intent(this, PlaylistListActivity::class.java)
                    .putExtra(PlaylistListActivity.EXTRA_MODE, PlaylistListActivity.MODE_LOAD)
            )
        }
        b.btnDeletePlaylist.setOnClickListener {
            playlistListLauncher.launch(
                Intent(this, PlaylistListActivity::class.java)
                    .putExtra(PlaylistListActivity.EXTRA_MODE, PlaylistListActivity.MODE_DELETE)
            )
        }
        b.btnClearQueue.setOnClickListener { confirmClearQueue() }
    }

    private fun confirmClearQueue() {
        MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
            .setMessage(R.string.playlist_clear_confirm)
            .setPositiveButton(R.string.playlist_clear_confirm_button) { _, _ ->
                vm.library.clearQueue()
                finish()
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    // ── Save Playlist ────────────────────────────────────────────────────────
    private fun showSaveDialog(queueEmpty: Boolean) {
        val dialogBinding = DialogSavePlaylistBinding.inflate(LayoutInflater.from(this))
        dialogBinding.tvEmptyQueueWarning.visibility = if (queueEmpty) android.view.View.VISIBLE else android.view.View.GONE

        val dialog = MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
            .setTitle(R.string.playlist_save)
            .setView(dialogBinding.root)
            .setPositiveButton(R.string.playlist_save_tracks, null)
            .setNegativeButton(R.string.cancel, null)
            .create()

        dialog.setOnShowListener {
            val saveButton = dialog.getButton(AlertDialog.BUTTON_POSITIVE)
            saveButton.isEnabled = false

            fun validate(): String? {
                val name = dialogBinding.etPlaylistName.text?.toString()?.trim().orEmpty()
                return when {
                    name.isEmpty() -> null
                    name.contains("/") -> getString(R.string.playlist_name_error_slash)
                    name.length > MAX_PLAYLIST_NAME_LEN -> getString(R.string.playlist_name_error_too_long)
                    else -> ""
                }
            }

            fun refreshValidity() {
                val name = dialogBinding.etPlaylistName.text?.toString()?.trim().orEmpty()
                val error = validate()
                dialogBinding.tilPlaylistName.error = if (error.isNullOrEmpty()) null else error
                saveButton.isEnabled = !queueEmpty && name.isNotEmpty() && error.isNullOrEmpty()
            }

            dialogBinding.etPlaylistName.addTextChangedListener(object : android.text.TextWatcher {
                override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
                override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
                override fun afterTextChanged(s: android.text.Editable?) { refreshValidity() }
            })
            refreshValidity()

            saveButton.setOnClickListener {
                val name = dialogBinding.etPlaylistName.text?.toString()?.trim().orEmpty()
                saveButton.isEnabled = false
                fetchPlaylistNames(
                    onResult = { existingNames ->
                        if (existingNames.any { it.equals(name, ignoreCase = false) }) {
                            saveButton.isEnabled = true
                            showOverwriteConfirm(name)
                        } else {
                            performSave(name, overwrite = false, onDone = { dialog.dismiss() }, onCancelToSaveDialog = {
                                saveButton.isEnabled = true
                            })
                        }
                    },
                    onFailure = {
                        saveButton.isEnabled = true
                        showFetchFailedDialog { showSaveDialog(queueEmpty) }
                    }
                )
            }
        }

        dialog.show()
    }

    private fun showOverwriteConfirm(name: String) {
        MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
            .setMessage(getString(R.string.playlist_overwrite_confirm, name))
            .setPositiveButton(R.string.playlist_overwrite) { _, _ ->
                performSave(name, overwrite = true, onDone = {}, onCancelToSaveDialog = {})
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun performSave(name: String, overwrite: Boolean, onDone: () -> Unit, onCancelToSaveDialog: () -> Unit) {
        if (overwrite) vm.playlist.overwrite(name) else vm.playlist.save(name)
        awaitPlaylistOpResult(
            onResult = { result ->
                if (result.ok) {
                    onDone()
                    MaterialAlertDialogBuilder(this, R.style.Theme_TinyRemote_AlertDialog)
                        .setMessage(R.string.playlist_created)
                        .setPositiveButton(R.string.ok) { _, _ -> finish() }
                        .setCancelable(false)
                        .show()
                } else {
                    onCancelToSaveDialog()
                    showRetryCancelDialog(
                        message = getString(R.string.playlist_save_failed, result.message),
                        onRetry = { performSave(name, overwrite, onDone, onCancelToSaveDialog) }
                    )
                }
            },
            onFailure = {
                onCancelToSaveDialog()
                showRetryCancelDialog(
                    message = getString(R.string.playlist_save_failed, ""),
                    onRetry = { performSave(name, overwrite, onDone, onCancelToSaveDialog) }
                )
            }
        )
    }

    // ── Shared helpers ───────────────────────────────────────────────────────
    private fun fetchPlaylistNames(onResult: (List<String>) -> Unit, onFailure: () -> Unit) {
        vm.playlist.requestNames()
        lifecycleScope.launch {
            try {
                val entries = withTimeout(5000) { vm.playlist.nameEntries.first { it != null } }
                onResult(entries!!.map { it.name })
            } catch (e: TimeoutCancellationException) {
                onFailure()
            }
        }
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

    private fun showFetchFailedDialog(onRetry: () -> Unit) {
        showRetryCancelDialog(getString(R.string.playlist_fetch_failed), onRetry)
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
        const val EXTRA_QUEUE_EMPTY = "queue_empty"
        private const val MAX_PLAYLIST_NAME_LEN = 55
    }
}

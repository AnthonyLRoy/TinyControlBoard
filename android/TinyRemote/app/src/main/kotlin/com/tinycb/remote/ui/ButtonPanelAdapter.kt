package com.tinycb.remote.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.card.MaterialCardView
import com.tinycb.remote.databinding.ItemButtonPanelBinding
import com.tinycb.remote.databinding.ItemButtonPanelWideBinding
import com.tinycb.remote.model.BoardStatus
import com.tinycb.remote.model.ButtonDef
import com.tinycb.remote.R

class ButtonPanelAdapter(
    private val onButtonClick: (ButtonDef) -> Unit
) : ListAdapter<ButtonDef, ButtonPanelAdapter.ViewHolder>(DIFF) {

    private var currentStatus: BoardStatus? = null

    fun updateStatus(status: BoardStatus?) {
        currentStatus = status
        notifyItemRangeChanged(0, itemCount, PAYLOAD_LED)
    }

    override fun getItemViewType(position: Int): Int =
        if (getItem(position).spanSize > 1) VIEW_TYPE_WIDE else VIEW_TYPE_NORMAL

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        return if (viewType == VIEW_TYPE_WIDE) {
            val binding = ItemButtonPanelWideBinding.inflate(inflater, parent, false)
            ViewHolder(binding.root, binding.ivIcon, binding.tvLabel, binding.ledDot)
        } else {
            val binding = ItemButtonPanelBinding.inflate(inflater, parent, false)
            ViewHolder(binding.root, binding.ivIcon, binding.tvLabel, binding.ledDot)
        }
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        holder.bind(getItem(position), currentStatus)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int, payloads: List<Any>) {
        if (payloads.contains(PAYLOAD_LED)) {
            holder.updateLed(getItem(position), currentStatus)
        } else {
            super.onBindViewHolder(holder, position, payloads)
        }
    }

    inner class ViewHolder(
        private val root: MaterialCardView,
        private val ivIcon: ImageView,
        private val tvLabel: TextView,
        private val ledDot: View
    ) : RecyclerView.ViewHolder(root) {

        fun bind(btn: ButtonDef, status: BoardStatus?) {
            ivIcon.setImageResource(btn.iconRes)
            tvLabel.text = btn.name
            root.setCardBackgroundColor(
                ContextCompat.getColor(root.context, btn.backgroundColorRes)
            )
            root.setOnClickListener { onButtonClick(btn) }
            updateLed(btn, status)
        }

        fun updateLed(btn: ButtonDef, status: BoardStatus?) {
            val isActive = btn.bitmaskBit >= 0 &&
                    status != null &&
                    (status.buttonLedBitmask and (1 shl btn.bitmaskBit)) != 0
            ledDot.setBackgroundResource(
                if (isActive) R.drawable.led_dot_active else R.drawable.led_dot
            )
        }
    }

    companion object {
        private const val VIEW_TYPE_NORMAL = 0
        private const val VIEW_TYPE_WIDE = 1
        private const val PAYLOAD_LED = "LED"

        private val DIFF = object : DiffUtil.ItemCallback<ButtonDef>() {
            override fun areItemsTheSame(a: ButtonDef, b: ButtonDef) = a.index == b.index
            override fun areContentsTheSame(a: ButtonDef, b: ButtonDef) = a == b
        }
    }
}

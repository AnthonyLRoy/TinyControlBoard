package com.tinycb.remote.ui

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.databinding.ItemButtonPanelBinding
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

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val binding = ItemButtonPanelBinding.inflate(
            LayoutInflater.from(parent.context), parent, false
        )
        return ViewHolder(binding)
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

    inner class ViewHolder(private val b: ItemButtonPanelBinding) :
        RecyclerView.ViewHolder(b.root) {

        fun bind(btn: ButtonDef, status: BoardStatus?) {
            b.ivIcon.setImageResource(btn.iconRes)
            b.tvLabel.text = btn.name
            b.root.setOnClickListener { onButtonClick(btn) }
            updateLed(btn, status)
        }

        fun updateLed(btn: ButtonDef, status: BoardStatus?) {
            val isActive = btn.bitmaskBit >= 0 &&
                    status != null &&
                    (status.buttonLedBitmask and (1 shl btn.bitmaskBit)) != 0
            b.ledDot.setBackgroundResource(
                if (isActive) R.drawable.led_dot_active else R.drawable.led_dot
            )
        }
    }

    companion object {
        private const val PAYLOAD_LED = "LED"

        private val DIFF = object : DiffUtil.ItemCallback<ButtonDef>() {
            override fun areItemsTheSame(a: ButtonDef, b: ButtonDef) = a.index == b.index
            override fun areContentsTheSame(a: ButtonDef, b: ButtonDef) = a == b
        }
    }
}

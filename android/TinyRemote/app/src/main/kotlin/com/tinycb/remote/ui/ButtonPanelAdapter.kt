package com.tinycb.remote.ui

import android.content.res.ColorStateList
import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.R
import com.tinycb.remote.data.ButtonCatalog
import com.tinycb.remote.databinding.ItemButtonPanelBinding
import com.tinycb.remote.databinding.ItemButtonPanelWideBinding
import com.tinycb.remote.databinding.ItemBrightnessStepperBinding
import com.tinycb.remote.databinding.ItemGroupHeaderBinding
import com.tinycb.remote.model.BoardStatus
import com.tinycb.remote.model.ButtonDef
import com.tinycb.remote.model.GridItem

class ButtonPanelAdapter(
    private val onButtonClick: (commandId: Int) -> Unit
) : ListAdapter<GridItem, RecyclerView.ViewHolder>(DIFF) {

    private var currentStatus: BoardStatus? = null

    fun updateStatus(status: BoardStatus?) {
        currentStatus = status
        notifyItemRangeChanged(0, itemCount, PAYLOAD_LED)
    }

    override fun getItemViewType(position: Int): Int = when (val item = getItem(position)) {
        is GridItem.Header  -> VIEW_TYPE_HEADER
        is GridItem.Stepper -> VIEW_TYPE_STEPPER
        is GridItem.Button  -> if (item.def.spanSize > 1) VIEW_TYPE_WIDE else VIEW_TYPE_NORMAL
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        return when (viewType) {
            VIEW_TYPE_HEADER  -> HeaderViewHolder(ItemGroupHeaderBinding.inflate(inflater, parent, false))
            VIEW_TYPE_STEPPER -> StepperViewHolder(ItemBrightnessStepperBinding.inflate(inflater, parent, false))
            VIEW_TYPE_WIDE    -> {
                val b = ItemButtonPanelWideBinding.inflate(inflater, parent, false)
                ButtonViewHolder(b.root, b.ivIcon, b.tvLabel, b.ledDot)
            }
            else -> {
                val b = ItemButtonPanelBinding.inflate(inflater, parent, false)
                ButtonViewHolder(b.root, b.ivIcon, b.tvLabel, b.ledDot)
            }
        }
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        when (val item = getItem(position)) {
            is GridItem.Header  -> (holder as HeaderViewHolder).bind(item)
            is GridItem.Stepper -> (holder as StepperViewHolder).bind(item)
            is GridItem.Button  -> (holder as ButtonViewHolder).bind(item.def, currentStatus)
        }
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int, payloads: List<Any>) {
        if (payloads.contains(PAYLOAD_LED)) {
            // Only ButtonViewHolders track LED state; Headers and Steppers are static
            if (holder is ButtonViewHolder) {
                (getItem(position) as? GridItem.Button)?.let { holder.updateLed(it.def, currentStatus) }
            }
        } else {
            super.onBindViewHolder(holder, position, payloads)
        }
    }

    inner class HeaderViewHolder(private val b: ItemGroupHeaderBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(header: GridItem.Header) { b.tvGroupHeader.text = header.title }
    }

    inner class StepperViewHolder(private val b: ItemBrightnessStepperBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(stepper: GridItem.Stepper) {
            b.ivIcon.setImageResource(stepper.iconRes)
            b.btnDec.setOnClickListener { onButtonClick(stepper.decCommandId) }
            b.btnInc.setOnClickListener { onButtonClick(stepper.incCommandId) }
        }
    }

    inner class ButtonViewHolder(
        private val root: View,
        private val ivIcon: ImageView,
        private val tvLabel: TextView,
        private val ledDot: View
    ) : RecyclerView.ViewHolder(root) {

        fun bind(btn: ButtonDef, status: BoardStatus?) {
            ivIcon.setImageResource(btn.iconRes)
            ivIcon.contentDescription = btn.name
            
            // Adjust icon size for wide buttons when label is hidden
            if (btn.spanSize > 1) {
                val sizeDp = if (btn.showLabel) 24 else 28
                val px = android.util.TypedValue.applyDimension(
                    android.util.TypedValue.COMPLEX_UNIT_DIP,
                    sizeDp.toFloat(),
                    root.resources.displayMetrics
                ).toInt()
                
                val lp = ivIcon.layoutParams
                if (lp.width != px || lp.height != px) {
                    lp.width = px
                    lp.height = px
                    ivIcon.layoutParams = lp
                }
            }

            tvLabel.text = btn.name
            tvLabel.visibility = if (btn.showLabel) View.VISIBLE else View.GONE
            ledDot.visibility = if (btn.showLabel) View.VISIBLE else View.GONE
            root.setOnClickListener { onButtonClick(btn.commandId) }
            updateLed(btn, status)
        }

        fun updateLed(btn: ButtonDef, status: BoardStatus?) {
            val rawLedOn = if (btn.commandId == ButtonCatalog.CMD_PLAY) {
                status?.isTrackPlaying == true
            } else {
                btn.bitmaskBit >= 0 &&
                        status != null &&
                        (status.buttonLedBitmask and (1 shl btn.bitmaskBit)) != 0
            }

            // Display toggle LED reports the opposite of desired UI highlight.
            // We want green when the monitor is ON.
            val isActive = if (btn.commandId == ButtonCatalog.CMD_DISPLAY_OFF) !rawLedOn else rawLedOn
            ledDot.setBackgroundResource(if (isActive) R.drawable.led_dot_active else R.drawable.led_dot)

            if (btn.isToggle && isActive && btn.showLabel) {
                // Active toggle: dark card with accent border, accent icon + white label
                root.setBackgroundResource(R.drawable.bg_button_active)
                ivIcon.imageTintList = ColorStateList.valueOf(ContextCompat.getColor(root.context, R.color.menu_accent))
                tvLabel.setTextColor(Color.WHITE)
            } else {
                root.setBackgroundResource(R.drawable.bg_button_aluminium)
                tvLabel.setTextColor(Color.WHITE)
                val tint = when {
                    btn.isPrimary -> ContextCompat.getColor(root.context, R.color.menu_accent)
                    isActive      -> ContextCompat.getColor(root.context, R.color.menu_accent)
                    else          -> ContextCompat.getColor(root.context, R.color.menu_icon_default)
                }
                ivIcon.imageTintList = ColorStateList.valueOf(tint)
            }
        }
    }

    companion object {
        private const val VIEW_TYPE_HEADER  = 0
        private const val VIEW_TYPE_NORMAL  = 1
        private const val VIEW_TYPE_WIDE    = 2
        private const val VIEW_TYPE_STEPPER = 3
        private const val PAYLOAD_LED = "LED"

        private val DIFF = object : DiffUtil.ItemCallback<GridItem>() {
            override fun areItemsTheSame(a: GridItem, b: GridItem) = when {
                a is GridItem.Header  && b is GridItem.Header  -> a.title == b.title
                a is GridItem.Button  && b is GridItem.Button  -> a.def.index == b.def.index
                a is GridItem.Stepper && b is GridItem.Stepper -> a.decCommandId == b.decCommandId
                else -> false
            }
            override fun areContentsTheSame(a: GridItem, b: GridItem) = a == b
        }
    }
}

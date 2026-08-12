package com.tinycb.remote.ui

import android.content.res.ColorStateList
import android.graphics.Color
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ItemViewSelectionButtonBinding

data class ViewOption(
    val name: String,
    val subtitle: String,
    val iconRes: Int,
    val commandId: Int
)

class ViewSelectionAdapter(
    private val onSelected: (ViewOption) -> Unit
) : ListAdapter<ViewOption, ViewSelectionAdapter.VH>(DIFF) {

    private var selectedCommandId: Int? = null

    fun setSelected(commandId: Int?) {
        val oldId = selectedCommandId
        selectedCommandId = commandId
        // Simple refresh: find positions if possible, or just notify all
        notifyDataSetChanged()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH =
        VH(ItemViewSelectionButtonBinding.inflate(LayoutInflater.from(parent.context), parent, false))

    override fun onBindViewHolder(holder: VH, position: Int) =
        holder.bind(getItem(position))

    inner class VH(private val b: ItemViewSelectionButtonBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(item: ViewOption) {
            b.tvTitle.text = item.name
            b.tvSubtitle.text = item.subtitle
            b.ivIcon.setImageResource(item.iconRes)

            val isSelected = item.commandId == selectedCommandId

            b.rootContainer.setBackgroundResource(
                if (isSelected) R.drawable.bg_menu_item_selected else R.drawable.bg_menu_item_normal
            )

            val iconTint = if (isSelected)
                ContextCompat.getColor(itemView.context, R.color.menu_accent)
            else
                ContextCompat.getColor(itemView.context, R.color.menu_icon_default)

            val iconBg = if (isSelected)
                ContextCompat.getColor(itemView.context, R.color.menu_accent_dim)
            else
                Color.parseColor("#10FFFFFF")

            b.ivIcon.imageTintList = ColorStateList.valueOf(iconTint)
            b.iconChip.backgroundTintList = ColorStateList.valueOf(iconBg)
            b.radioButton.isChecked = isSelected

            b.root.setOnClickListener { onSelected(item) }
        }
    }

    companion object {
        private val DIFF = object : DiffUtil.ItemCallback<ViewOption>() {
            override fun areItemsTheSame(a: ViewOption, b: ViewOption) = a.commandId == b.commandId
            override fun areContentsTheSame(a: ViewOption, b: ViewOption) = a == b
        }
    }
}

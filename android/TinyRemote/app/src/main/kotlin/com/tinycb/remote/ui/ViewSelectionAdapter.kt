package com.tinycb.remote.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.tinycb.remote.R
import com.tinycb.remote.databinding.ItemViewSelectionButtonBinding

data class ViewOption(val name: String, val commandId: Int)

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
            b.tvViewName.text = item.name
            val isSelected = item.commandId == selectedCommandId
            
            b.cardRoot.strokeColor = if (isSelected) {
                ContextCompat.getColor(b.root.context, R.color.mcintosh_green)
            } else {
                android.graphics.Color.TRANSPARENT
            }
            
            b.ivCheck.visibility = if (isSelected) View.VISIBLE else View.INVISIBLE
            
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

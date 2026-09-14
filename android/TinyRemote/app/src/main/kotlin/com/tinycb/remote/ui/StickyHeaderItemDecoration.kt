package com.tinycb.remote.ui

import android.graphics.Canvas
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView

/**
 * Pins the topmost visible header view (as reported by [isHeader]) to the top of the
 * RecyclerView while its group's items scroll underneath, mimicking CSS `position: sticky`.
 */
class StickyHeaderItemDecoration(
    private val isHeader: (position: Int) -> Boolean
) : RecyclerView.ItemDecoration() {

    private val headerCache = HashMap<Int, View>()

    override fun onDrawOver(c: Canvas, parent: RecyclerView, state: RecyclerView.State) {
        val topChild = parent.findChildViewUnder(parent.paddingLeft.toFloat(), parent.paddingTop.toFloat()) ?: return
        val topPosition = parent.getChildAdapterPosition(topChild)
        if (topPosition == RecyclerView.NO_POSITION) return

        val headerPosition = findHeaderPositionForItem(topPosition)
        if (headerPosition == -1) return

        val headerView = getHeaderViewForItem(headerPosition, parent)
        val nextHeaderPosition = findNextHeaderPosition(headerPosition, parent)
        val contactPoint = headerView.bottom
        val childInContact = getChildInContact(parent, contactPoint, nextHeaderPosition)

        c.save()
        val translation = if (childInContact != null && isHeader(parent.getChildAdapterPosition(childInContact))) {
            (childInContact.top - headerView.height).toFloat()
        } else 0f
        c.translate(0f, translation)
        headerView.draw(c)
        c.restore()
    }

    /** Call when the underlying list data changes so stale cached header views are rebuilt. */
    fun invalidateCache() = headerCache.clear()

    private fun findHeaderPositionForItem(itemPosition: Int): Int {
        var position = itemPosition
        while (position >= 0) {
            if (isHeader(position)) return position
            position--
        }
        return -1
    }

    private fun findNextHeaderPosition(afterPosition: Int, parent: RecyclerView): Int {
        val itemCount = parent.adapter?.itemCount ?: return -1
        var position = afterPosition + 1
        while (position < itemCount) {
            if (isHeader(position)) return position
            position++
        }
        return -1
    }

    private fun getChildInContact(parent: RecyclerView, contactPoint: Int, nextHeaderPosition: Int): View? {
        for (i in 0 until parent.childCount) {
            val child = parent.getChildAt(i)
            val heightTolerance = if (nextHeaderPosition != -1 &&
                parent.getChildAdapterPosition(child) == nextHeaderPosition
            ) child.height - (child.bottom - contactPoint) else 0
            if (child.bottom - heightTolerance > contactPoint && child.top <= contactPoint) {
                return child
            }
        }
        return null
    }

    private fun getHeaderViewForItem(headerPosition: Int, parent: RecyclerView): View {
        headerCache[headerPosition]?.let { return it }
        val adapter = parent.adapter ?: throw IllegalStateException("Adapter is null")
        val viewType = adapter.getItemViewType(headerPosition)
        @Suppress("UNCHECKED_CAST")
        val typedAdapter = adapter as RecyclerView.Adapter<RecyclerView.ViewHolder>
        val holder = typedAdapter.createViewHolder(parent, viewType)
        typedAdapter.onBindViewHolder(holder, headerPosition)
        val view = holder.itemView
        fixLayoutSize(parent, view)
        headerCache[headerPosition] = view
        return view
    }

    private fun fixLayoutSize(parent: RecyclerView, view: View) {
        val widthSpec = View.MeasureSpec.makeMeasureSpec(parent.width, View.MeasureSpec.EXACTLY)
        val heightSpec = View.MeasureSpec.makeMeasureSpec(parent.height, View.MeasureSpec.UNSPECIFIED)

        val childWidthSpec = ViewGroup.getChildMeasureSpec(
            widthSpec, view.paddingLeft + view.paddingRight, view.layoutParams?.width ?: ViewGroup.LayoutParams.MATCH_PARENT
        )
        val childHeightSpec = ViewGroup.getChildMeasureSpec(
            heightSpec, view.paddingTop + view.paddingBottom, view.layoutParams?.height ?: ViewGroup.LayoutParams.WRAP_CONTENT
        )

        view.measure(childWidthSpec, childHeightSpec)
        view.layout(0, 0, view.measuredWidth, view.measuredHeight)
    }
}

package com.tinycb.remote.model

sealed class GridItem {
    abstract val spanSize: Int

    data class Header(val title: String) : GridItem() {
        override val spanSize: Int = 4
    }

    data class Button(val def: ButtonDef) : GridItem() {
        override val spanSize: Int get() = def.spanSize
    }

    /** Full-width stepper for controls that increment/decrement a value (e.g. brightness). */
    data class Stepper(
        val title: String,
        val iconRes: Int,
        val decCommandId: Int,
        val incCommandId: Int
    ) : GridItem() {
        override val spanSize: Int = 4
    }
}

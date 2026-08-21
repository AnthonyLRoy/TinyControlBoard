package com.tinycb.remote.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.BlendMode
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.ImageShader
import androidx.compose.ui.graphics.ShaderBrush
import androidx.compose.ui.graphics.TileMode
import androidx.compose.ui.res.imageResource
import androidx.compose.ui.tooling.preview.Preview
import com.tinycb.remote.R

enum class MetallicShade {
    Silver,
    Titanium,
    Gunmetal
}

@Composable
fun BrushedAluminumSurface(
    shade: MetallicShade = MetallicShade.Titanium,
    modifier: Modifier = Modifier
) {
    val aluminumColors = when (shade) {
        MetallicShade.Silver -> listOf(
            Color(0xFFE0E0E0), Color(0xFFBDBDBD), Color(0xFFEEEEEE), Color(0xFF9E9E9E), Color(0xFFE0E0E0)
        )
        MetallicShade.Titanium -> listOf(
            Color(0xFF353535), Color(0xFF1F1F1F), Color(0xFF484848), Color(0xFF121212), Color(0xFF353535)
        )
        MetallicShade.Gunmetal -> listOf(
            Color(0xFF1A1A1A), Color(0xFF0D0D0D), Color(0xFF282828), Color(0xFF050505), Color(0xFF1A1A1A)
        )
    }

    val noiseBitmap = ImageBitmap.imageResource(id = R.drawable.noise_grain)
    val noiseShader = ImageShader(noiseBitmap, TileMode.Repeated, TileMode.Repeated)
    val noiseBrush = ShaderBrush(noiseShader)

    Canvas(modifier = modifier.fillMaxSize()) {
        // 1. Draw the metallic gradient base
        drawRect(
            brush = Brush.linearGradient(
                colors = aluminumColors,
                start = androidx.compose.ui.geometry.Offset(0f, 0f),
                end = androidx.compose.ui.geometry.Offset(size.width, size.height)
            )
        )

        // 2. Overlay the brushed grain texture
        drawRect(
            brush = noiseBrush,
            alpha = 0.35f,
            blendMode = BlendMode.Overlay
        )
        
        // 3. Subtle edge vignette to emphasize the 3D look
        drawRect(
            brush = Brush.radialGradient(
                colors = listOf(Color.Transparent, Color(0x66000000)),
                center = center,
                radius = size.minDimension
            ),
            blendMode = BlendMode.Multiply
        )
    }
}

@Preview(showBackground = true)
@Composable
fun TitaniumBackgroundPreview() {
    BrushedAluminumSurface(shade = MetallicShade.Titanium)
}

@Preview(showBackground = true)
@Composable
fun SilverBackgroundPreview() {
    BrushedAluminumSurface(shade = MetallicShade.Silver)
}

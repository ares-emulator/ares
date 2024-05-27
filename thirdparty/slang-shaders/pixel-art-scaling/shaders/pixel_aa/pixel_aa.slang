#version 450

/*
    Pixel AA v1.5 by fishku
    Copyright (C) 2023
    Public domain license (CC0)

    Features:
    - Sharp upscaling with anti-aliasing
    - Subpixel upscaling
    - Sharpness can be controlled
    - Gamma correct blending
    - Integer scales result in pixel-perfect scaling
    - Can use bilinear filtering for max. performance

    Inspired by:
    https://www.shadertoy.com/view/MlB3D3
    by d7samurai
    and:
    https://www.youtube.com/watch?v=d6tp43wZqps
    by t3ssel8r

    With sharpness = 1.0, using the same gamma-correct blending, and disabling
    subpixel anti-aliasing, results are identical to the "pixellate" shader.

    Changelog:
    v1.5: Upstream optimizations from GLSL port. Add free transform preset.
    v1.4: Enable subpixel sampling for all four pixel layout orientations,
          including rotated screens.
    v1.3: Account for screen rotation in subpixel sampling.
    v1.2: Optimize and simplify algorithm. Enable sharpness < 1.0. Fix subpixel
          sampling bug.
    v1.1: Better subpixel sampling.
    v1.0: Initial release.
*/

#include "parameters.inc"
#include "shared.inc"

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OutputSize;
    uint Rotation;
    float PIX_AA_SHARP;
    float PIX_AA_GAMMA;
    float PIX_AA_SUBPX;
    float PIX_AA_SUBPX_ORIENTATION;
}
param;

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; }
global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 tx_coord;
layout(location = 1) out vec2 tx_per_px;
layout(location = 2) out vec2 tx_to_uv;

void main() {
    gl_Position = global.MVP * Position;
    tx_coord = TexCoord * param.SourceSize.xy;
    tx_per_px = param.SourceSize.xy * param.OutputSize.zw;
    tx_to_uv = param.SourceSize.zw;
}

#pragma stage fragment
layout(location = 0) in vec2 tx_coord;
layout(location = 1) in vec2 tx_per_px;
layout(location = 2) in vec2 tx_to_uv;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main() {
    FragColor =
        pixel_aa(Source, tx_per_px, tx_to_uv, tx_coord, param.PIX_AA_SHARP,
                 param.PIX_AA_GAMMA > 0.5, param.PIX_AA_SUBPX > 0.5,
                 uint(param.PIX_AA_SUBPX_ORIENTATION), param.Rotation);
}

#version 450

/*
    Authentic GBC v1.0 by fishku
    Copyright (C) 2024
    Public domain license (CC0)

    Attempts to render GBC subpixels authentically.

    Reference photos:
    - https://gbcc.dev/technology/subpixels.jpg

    Inspired by:
    -
   https://www.reddit.com/r/AnaloguePocket/comments/1azaxgd/ive_made_some_improvements_to_my_analogue_pocket/

    Changelog:
    v1.0: Initial release.
*/

#include "parameters.inc"
#include "shared.inc"

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OutputSize;
    float AUTH_GBC_BRIG;
}
param;

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; }
global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec4 px_rect;
layout(location = 1) out vec2 tx_coord;
layout(location = 2) out vec2 tx_to_px;
layout(location = 3) out vec2 subpx_size;
layout(location = 4) out vec2 notch_size;
layout(location = 5) out float subpx_orig_y;

void main() {
    gl_Position = global.MVP * Position;

    auth_gbc_vert_shader(param.SourceSize.xy, param.OutputSize.xy,
                         param.AUTH_GBC_BRIG, TexCoord, px_rect, tx_coord,
                         tx_to_px, subpx_size, notch_size, subpx_orig_y);
}

#pragma stage fragment
layout(location = 0) in vec4 px_rect;
layout(location = 1) in vec2 tx_coord;
layout(location = 2) in vec2 tx_to_px;
layout(location = 3) in vec2 subpx_size;
layout(location = 4) in vec2 notch_size;
layout(location = 5) in float subpx_orig_y;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main() {
#if 0
    // Debug
    vec3 res = pixel_color(px_rect, vec2(500.0), vec2(100.0));
    res += pixel_color(px_rect, vec2(500.0), vec2(600.0, 100.0));
    res += pixel_color(px_rect, vec2(500.0), vec2(100.0, 600.0));
    res += pixel_color(px_rect, vec2(500.0), vec2(600.0));
    FragColor = vec4(res, 1.0);
#else
    // Figure out 4 nearest texels in source texture
    vec2 tx_coord_i;
    const vec2 tx_coord_f = modf(tx_coord, tx_coord_i);
    const vec2 tx_coord_off = step(vec2(0.5), tx_coord_f) * 2.0 - 1.0;
    const vec2 tx_origins[] = {
        tx_coord_i, tx_coord_i + vec2(tx_coord_off.x, 0.0),
        tx_coord_i + vec2(0.0, tx_coord_off.y), tx_coord_i + tx_coord_off};

    // Sample.
    // Apply square for fast "gamma correction".
    vec3 samples[] = {
        texture(Source, (tx_origins[0] + 0.5) * param.SourceSize.zw).rgb,
        texture(Source, (tx_origins[1] + 0.5) * param.SourceSize.zw).rgb,
        texture(Source, (tx_origins[2] + 0.5) * param.SourceSize.zw).rgb,
        texture(Source, (tx_origins[3] + 0.5) * param.SourceSize.zw).rgb};
    samples[0] *= samples[0];
    samples[1] *= samples[1];
    samples[2] *= samples[2];
    samples[3] *= samples[3];

    // Apply shader.
    const vec3 res =
        samples[0] * pixel_color(px_rect, tx_to_px, tx_origins[0] * tx_to_px,
                                 subpx_orig_y, subpx_size, notch_size) +
        samples[1] * pixel_color(px_rect, tx_to_px, tx_origins[1] * tx_to_px,
                                 subpx_orig_y, subpx_size, notch_size) +
        samples[2] * pixel_color(px_rect, tx_to_px, tx_origins[2] * tx_to_px,
                                 subpx_orig_y, subpx_size, notch_size) +
        samples[3] * pixel_color(px_rect, tx_to_px, tx_origins[3] * tx_to_px,
                                 subpx_orig_y, subpx_size, notch_size);

    // Apply sqrt for fast "gamma correction".
    FragColor = vec4(sqrt(res), 1.0);
#endif
}

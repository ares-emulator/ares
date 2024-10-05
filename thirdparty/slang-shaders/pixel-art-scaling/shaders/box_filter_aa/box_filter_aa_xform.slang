#version 450

/*
    Box Filter AA v1.0 by fishku
    Copyright (C) 2024
    Public domain license (CC0)

    Branching-free anti-aliasing using pixel coverage.

    Changelog:
    v1.0: Initial release.
*/

// clang-format off
#include "../../../misc/shaders/input_transform/parameters.inc"

#include "../../../misc/shaders/coverage/coverage.inc"
#include "../../../misc/shaders/input_transform/input_transform.inc"
// clang-format on

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OutputSize;
    uint Rotation;
    // From input transform library, scaling section
    float FORCE_ASPECT_RATIO;
    float ASPECT_H;
    float ASPECT_V;
    float FORCE_INTEGER_SCALING_H;
    float FORCE_INTEGER_SCALING_V;
    float OVERSCALE;
    // From input transform library, cropping section
    float OS_CROP_TOP;
    float OS_CROP_BOTTOM;
    float OS_CROP_LEFT;
    float OS_CROP_RIGHT;
    // From input transform library, moving section
    float SHIFT_H;
    float SHIFT_V;
    float CENTER_AFTER_CROPPING;
}
param;

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; }
global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec4 px_rect;
layout(location = 1) out vec4 input_corners;
layout(location = 2) out vec2 scale_i2o;
layout(location = 3) out vec2 input_center;
layout(location = 4) out vec2 output_center;
layout(location = 5) out vec2 tx_coord;
void main() {
    gl_Position = global.MVP * Position;

    px_rect = vec4(TexCoord * param.OutputSize.xy - 0.5,
                   TexCoord * param.OutputSize.xy + 0.5);
    const vec4 crop = vec4(param.OS_CROP_TOP, param.OS_CROP_LEFT,
                           param.OS_CROP_BOTTOM, param.OS_CROP_RIGHT);
    scale_i2o = get_scale_i2o(
        param.SourceSize.xy, param.OutputSize.xy, crop, param.Rotation,
        param.CENTER_AFTER_CROPPING, param.FORCE_ASPECT_RATIO,
        vec2(param.ASPECT_H, param.ASPECT_V),
        vec2(param.FORCE_INTEGER_SCALING_H, param.FORCE_INTEGER_SCALING_V),
        param.OVERSCALE);
    const vec2 shift = vec2(param.SHIFT_H, param.SHIFT_V);
    input_center = get_input_center(param.SourceSize.xy, param.OutputSize.xy,
                                    scale_i2o, crop, shift, param.Rotation,
                                    param.CENTER_AFTER_CROPPING);
    tx_coord = transform(TexCoord, vec2(0.5), param.OutputSize.xy / scale_i2o,
                         input_center);
    output_center = 0.5 * param.OutputSize.xy;
    input_corners =
        get_input_corners(param.SourceSize.xy, crop, param.Rotation);
}

#pragma stage fragment
layout(location = 0) in vec4 px_rect;
layout(location = 1) in vec4 input_corners;
layout(location = 2) in vec2 scale_i2o;
layout(location = 3) in vec2 input_center;
layout(location = 4) in vec2 output_center;
layout(location = 5) in vec2 tx_coord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main() {
    // Figure out 4 nearest texels in source texture.
    // Clamp tx_coord for proper cropping.
    vec2 tx_coord_i;
    const vec2 tx_coord_f =
        modf(clamp(tx_coord, input_corners.xy, input_corners.zw), tx_coord_i);
    const vec2 tx_coord_off = step(vec2(0.5), tx_coord_f) * 2.0 - 1.0;
    vec2 tx_origins[] = {tx_coord_i, tx_coord_i + vec2(tx_coord_off.x, 0.0),
                         tx_coord_i + vec2(0.0, tx_coord_off.y),
                         tx_coord_i + tx_coord_off};

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
    // Transform tx_origins into pixel output space.
    tx_origins[0] =
        transform(tx_origins[0], input_center, scale_i2o, output_center);
    tx_origins[1] =
        transform(tx_origins[1], input_center, scale_i2o, output_center);
    tx_origins[2] =
        transform(tx_origins[2], input_center, scale_i2o, output_center);
    tx_origins[3] =
        transform(tx_origins[3], input_center, scale_i2o, output_center);

    const vec3 res =
        samples[0] * rect_coverage(px_rect, vec4(tx_origins[0],
                                                 tx_origins[0] + scale_i2o)) +
        samples[1] * rect_coverage(px_rect, vec4(tx_origins[1],
                                                 tx_origins[1] + scale_i2o)) +
        samples[2] * rect_coverage(px_rect, vec4(tx_origins[2],
                                                 tx_origins[2] + scale_i2o)) +
        samples[3] * rect_coverage(px_rect, vec4(tx_origins[3],
                                                 tx_origins[3] + scale_i2o));

    // Apply sqrt for fast "gamma correction".
    FragColor = vec4(sqrt(res), 1.0);
}

#version 450

/*
    Average fill v1.8 by fishku
    Copyright (C) 2023
    Public domain license (CC0)

    This shader preset allows cropping the image on any side, and filling the
    cropped area with the average color of an adjustable area next to it.
    This is useful for certain games that do not render a full image to maintain
    the overall aspect ratio and to avoid burn-in.

    The preset also allows you to extend the original content to a larger
    screen. It's recommended to set the video scaling options as follows:
    - Turn integer scaling OFF
    - Set aspect ratio to FULL
    The shader will then take over and handle the proper scaling and aspect
    ratio of the input.

    In case the image is cropped on multiple sides, different blend modes for
    the corner are available. Simply change the parameter for the "corner blend
    mode".
    The available corner blend modes are:
    0 = Draw horizontal bars on top
    1 = Draw vertical bars on top
    2 = Blend bars by weighted averaging
    3 = Smooth angle-based blending

    Changelog:
    v1.8: Add shift option from input transform library.
    v1.7: Add overscale option from crop and scale library.
    v1.6: Refactor for new scaling library. Add rotation support.
    v1.5: Optimize. Update to new Pixel AA version.
    v1.4: Add anti-aliased interpolation for non-integer scaling.
    v1.3: Fix scaling bugs.
    v1.2: Fix scaling bugs.
    v1.1: Add extension modes from blur fill; Add average gamma adjustment.
    v1.0: Initial release.
*/

// clang-format off
#include "parameters.inc"
#include "../../../pixel-art-scaling/shaders/pixel_aa/parameters.inc"
// clang-format on

#include "../../../misc/shaders/input_transform/input_transform.inc"
#include "../../../pixel-art-scaling/shaders/pixel_aa/shared.inc"

layout(push_constant) uniform Push {
    vec4 InputSize;
    vec4 OutputSize;
    uint Rotation;
    // Own settings
    float EXTEND_H;
    float EXTEND_V;
    float CORNER_BLEND_MODE;
    float FILL_GAMMA;
    float SAMPLE_SIZE;
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
    // From pixel AA
    float PIX_AA_SHARP;
    float PIX_AA_SUBPX;
    float PIX_AA_SUBPX_ORIENTATION;
}
param;

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; }
global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 scale_o2i;
layout(location = 2) out vec4 crop;
layout(location = 3) out vec2 shift;
layout(location = 4) out vec2 tx_coord;
layout(location = 5) out vec2 tx_per_px;
layout(location = 6) out vec2 tx_to_uv;
layout(location = 7) out vec4 input_corners;
layout(location = 8) out vec2 cropped_input_size;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    crop = vec4(param.OS_CROP_TOP, param.OS_CROP_LEFT, param.OS_CROP_BOTTOM,
                param.OS_CROP_RIGHT);
    scale_o2i = get_scale_o2i(
        param.InputSize.xy, param.OutputSize.xy, crop, param.Rotation,
        param.CENTER_AFTER_CROPPING, param.FORCE_ASPECT_RATIO,
        vec2(param.ASPECT_H, param.ASPECT_V),
        vec2(param.FORCE_INTEGER_SCALING_H, param.FORCE_INTEGER_SCALING_V),
        param.OVERSCALE,
        /* output_size_is_final_viewport_size = */ false);
    shift = vec2(param.SHIFT_H, param.SHIFT_V);
    tx_coord = o2i(vTexCoord, param.InputSize.xy, crop, shift, param.Rotation,
                   param.CENTER_AFTER_CROPPING, scale_o2i);
    tx_per_px = scale_o2i * param.OutputSize.zw;
    tx_to_uv = param.InputSize.zw;

    input_corners = get_input_corners(param.InputSize.xy, crop, param.Rotation);
    const vec4 rotated_crop = get_rotated_crop(crop, param.Rotation);
    cropped_input_size =
        param.InputSize.xy -
        vec2(rotated_crop.y + rotated_crop.w, rotated_crop.x + rotated_crop.z);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 scale_o2i;
layout(location = 2) in vec4 crop;
layout(location = 3) in vec2 shift;
layout(location = 4) in vec2 tx_coord;
layout(location = 5) in vec2 tx_per_px;
layout(location = 6) in vec2 tx_to_uv;
layout(location = 7) in vec4 input_corners;
layout(location = 8) in vec2 cropped_input_size;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Input;
layout(set = 0, binding = 3) uniform sampler2D Top;
layout(set = 0, binding = 4) uniform sampler2D Bottom;
layout(set = 0, binding = 5) uniform sampler2D Left;
layout(set = 0, binding = 6) uniform sampler2D Right;

#define PI 3.1415926538

// For mipmap sampling, use a big offset to get the average of a PoT input.
#define BIG_NUMBER 9000.1

vec3 blend_corner(vec3 a,      // The first color to blend
                  vec3 b,      // The second color to blend
                  float wa,    // The weight of the first color
                  float wb,    // The weight of the second color
                  vec2 coord,  // The coordinate to evaluate the blend for
                  vec2 corner_coord,  // The coordinate of the corner of the
                                      // content after cropping
                  vec2 gap_size  // The component-wise distance from the corner
                                 // of the content to the corner of the viewport
) {
    switch (int(param.CORNER_BLEND_MODE)) {
        case 0:
            // Horizontal bars on top
            return b;
        case 1:
            // Vertical bars on top
            return a;
        case 2:
            // Weighted average of averages
            return mix(a, b, wa / (wa + wb));
        case 3:
        default:
            // Angle blend
            const vec2 delta = (coord - corner_coord) / gap_size;
            // Use absolutes to always operate in 1st quadrant.
            // This makes the angle work out to be correct in all cases when
            // carefully choosing argument ordering.
            const float angle = atan(abs(delta.y), abs(delta.x)) / (PI * 0.5);
            // Smoothstep makes the transition perceptually smoother.
            return mix(a, b, smoothstep(0.0, 1.0, angle));
    }
}

void main() {
    const vec2 extend_fill =
        get_rotated_size(vec2(param.EXTEND_H, param.EXTEND_V), param.Rotation);
    if (tx_coord.x < input_corners.x) {
        if (extend_fill.x < 0.5) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
        const vec3 left = textureLod(Left, vec2(0.5), BIG_NUMBER).rgb;
        if (tx_coord.y < input_corners.y) {
            if (extend_fill.y < 0.5) {
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                return;
            }
            // Top left corner
            const vec3 top = textureLod(Top, vec2(0.5), BIG_NUMBER).rgb;
            const vec2 content_corner =
                i2o(input_corners.xy, param.InputSize.xy, crop, shift,
                    param.Rotation, param.CENTER_AFTER_CROPPING, scale_o2i);
            const vec2 viewport_corner = vec2(0.0, 0.0);
            FragColor = vec4(
                blend_corner(left, top, cropped_input_size.y,
                             cropped_input_size.x, vTexCoord, content_corner,
                             viewport_corner - content_corner),
                1.0);
            FragColor.rgb = pow(FragColor.rgb, vec3(param.FILL_GAMMA));
        } else if (tx_coord.y < input_corners.w) {
            // Left bar
            FragColor = vec4(pow(left, vec3(param.FILL_GAMMA)), 1.0);
        } else {
            if (extend_fill.y < 0.5) {
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                return;
            }
            // Bottom left corner
            const vec3 bottom = textureLod(Bottom, vec2(0.5), BIG_NUMBER).rgb;
            const vec2 content_corner =
                i2o(input_corners.xw, param.InputSize.xy, crop, shift,
                    param.Rotation, param.CENTER_AFTER_CROPPING, scale_o2i);
            const vec2 viewport_corner = vec2(0.0, 1.0);
            FragColor = vec4(
                blend_corner(left, bottom, cropped_input_size.y,
                             cropped_input_size.x, vTexCoord, content_corner,
                             viewport_corner - content_corner),
                1.0);
            FragColor.rgb = pow(FragColor.rgb, vec3(param.FILL_GAMMA));
        }
    } else if (tx_coord.x < input_corners.z) {
        if (tx_coord.y < input_corners.y) {
            if (extend_fill.y < 0.5) {
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                return;
            }
            // Top bar
            FragColor = vec4(textureLod(Top, vec2(0.5), BIG_NUMBER).rgb, 1.0);
            FragColor.rgb = pow(FragColor.rgb, vec3(param.FILL_GAMMA));
        } else if (tx_coord.y < input_corners.w) {
            // Uncropped
            if (param.FORCE_INTEGER_SCALING_H > 0.5 &&
                param.FORCE_INTEGER_SCALING_V > 0.5) {
                // Do a perfectly sharp (nearest neighbor) sampling.
                FragColor = vec4(
                    texture(Input, (floor(tx_coord) + 0.5) * param.InputSize.zw)
                        .rgb,
                    1.0);
            } else {
                // Do a sharp anti-aliased interpolation.
                // Do not correct for gamma additionally because the input is
                // already in linear color space.
                FragColor = pixel_aa(
                    Input, tx_per_px, tx_to_uv, tx_coord, param.PIX_AA_SHARP,
                    /* gamma_correct = */ false, param.PIX_AA_SUBPX > 0.5,
                    uint(param.PIX_AA_SUBPX_ORIENTATION), param.Rotation);
            }
        } else {
            if (extend_fill.y < 0.5) {
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                return;
            }
            // Bottom bar
            FragColor =
                vec4(textureLod(Bottom, vec2(0.5), BIG_NUMBER).rgb, 1.0);
            FragColor.rgb = pow(FragColor.rgb, vec3(param.FILL_GAMMA));
        }
    } else {
        if (extend_fill.x < 0.5) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
        const vec3 right = textureLod(Right, vec2(0.5), BIG_NUMBER).rgb;
        if (tx_coord.y < input_corners.y) {
            if (extend_fill.y < 0.5) {
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                return;
            }
            // Top right corner
            const vec3 top = textureLod(Top, vec2(0.5), BIG_NUMBER).rgb;
            const vec2 content_corner =
                i2o(input_corners.zy, param.InputSize.xy, crop, shift,
                    param.Rotation, param.CENTER_AFTER_CROPPING, scale_o2i);
            const vec2 viewport_corner = vec2(1.0, 0.0);
            FragColor = vec4(
                blend_corner(right, top, cropped_input_size.y,
                             cropped_input_size.x, vTexCoord, content_corner,
                             viewport_corner - content_corner),
                1.0);
            FragColor.rgb = pow(FragColor.rgb, vec3(param.FILL_GAMMA));
        } else if (tx_coord.y < input_corners.w) {
            // Right bar
            FragColor = vec4(pow(right, vec3(param.FILL_GAMMA)), 1.0);
        } else {
            if (extend_fill.y < 0.5) {
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                return;
            }
            // Bottom right corner
            const vec3 bottom = textureLod(Bottom, vec2(0.5), BIG_NUMBER).rgb;
            const vec2 content_corner =
                i2o(input_corners.zw, param.InputSize.xy, crop, shift,
                    param.Rotation, param.CENTER_AFTER_CROPPING, scale_o2i);
            const vec2 viewport_corner = vec2(1.0, 1.0);
            FragColor = vec4(
                blend_corner(right, bottom, cropped_input_size.y,
                             cropped_input_size.x, vTexCoord, content_corner,
                             viewport_corner - content_corner),
                1.0);
            FragColor.rgb = pow(FragColor.rgb, vec3(param.FILL_GAMMA));
        }
    }
}

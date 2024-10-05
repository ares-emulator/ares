/*
    Input transformation library v1.3 by fishku
    Copyright (C) 2023-2024
    Public domain license (CC0)

    Apply cropping, scaling, and transformation operations to input viewport and
    provide utility functions for coordinate mappings.

    This file acts like a library and should be included in another shader to be
    used. For example usages, see the border/blur_fill shaders.

    It's recommended to use these functions in the vertex shader pass, and pass
    the data to the fragment pass.

    Features:
    - Cropping on each side
    - Centering of image after crop has been applied
    - Additional translation in either direction
    - Forcing of a certain aspect ratio
    - Forcing of either vert. or horiz. integer scaling, or both
    - Rotation support (0, 90, 180, 270 degrees) -- all "vertical" and
      "horizontal" paramaters are transformed accordingly.
    - Overscaling

    Refactored from the version that used to be in the blur_fill shader.

    Changelog:
    v1.3: Bug fixes and cleanup.
    v1.2: Rename to "input transform". Add translation option.
    v1.1: Add overscaling option. Unify parameters.
    v1.0: Initial conversion from blur_fill release. Add rotation support.
*/

#include "rotation.inc"

// Get 2 corners of input in texel space, spanning the input image.
// corners.x and .y define the top-left corner, corners.z and .w define the
// bottom-right corner.
vec4 get_input_corners(vec2 input_size, vec4 crop, uint rotation) {
    crop = get_rotated_crop(crop, rotation);
    return vec4(crop.y, crop.x, input_size.x - crop.w, input_size.y - crop.z);
}

// Get adjusted center in input pixel (texel) coordinate system.
// Crop is in input pixels (texels).
// Shift is in output pixels.
vec2 get_input_center(vec2 input_size, vec2 output_size, vec2 scale_i2o,
                      vec4 crop, vec2 shift, uint rotation,
                      float center_after_cropping) {
    crop = get_rotated_crop(crop, rotation);
    shift = get_rotated_vector(shift, rotation);
    // If input and output sizes have different parity, shift by 1/2 of an
    // output pixel to avoid having input pixel (texel) edges on output pixel
    // centers, which leads to all sorts of issues.
    return 0.5 * (input_size + center_after_cropping *
                                   vec2(crop.y - crop.w, crop.x - crop.z)) +
           (0.5 * mod(input_size + output_size, 2.0) - shift) / scale_i2o;
}

// Scaling from input to output space.
vec2 get_scale_i2o(vec2 input_size, vec2 output_size, vec4 crop, uint rotation,
                   float center_after_cropping, float force_aspect_ratio,
                   vec2 aspect, vec2 force_integer_scaling, float overscale) {
    crop = get_rotated_crop(crop, rotation);
    aspect = get_rotated_size(aspect, rotation);
    // Aspect ratio before cropping.
    // Corrected for forced aspect ratio.
    aspect = (force_aspect_ratio < 0.5
                  ? output_size * input_size.yx
                  : (aspect.x < 0.5 || aspect.y < 0.5
                         ? vec2(1.0)
                         : vec2(aspect.x, aspect.y) * input_size.yx));
    // Pixels in input coord. space, after cropping.
    input_size = input_size -
                 (center_after_cropping > 0.5
                      ? vec2(crop.y + crop.w, crop.x + crop.z)
                      : 2.0 * vec2(min(crop.y, crop.w), min(crop.x, crop.z)));

    force_integer_scaling = get_rotated_size(force_integer_scaling, rotation);
    vec2 scale;
    if (output_size.x / (input_size.x * aspect.x) <
        output_size.y / (input_size.y * aspect.y)) {
        // Scale will be limited by width. Calc x scale, then derive y scale
        // using aspect ratio.
        scale.x = mix(output_size.x / input_size.x,
                      output_size.y * aspect.x / (input_size.y * aspect.y),
                      overscale);
        if (force_integer_scaling.x > 0.5 && scale.x > 1.0) {
            scale.x = floor(scale.x);
        }
        scale.y = scale.x * aspect.y / aspect.x;
        if (force_integer_scaling.y > 0.5 && scale.y > 1.0) {
            scale.y = floor(scale.y);
        }
    } else {
        // Scale will be limited by height.
        scale.y = mix(output_size.y / input_size.y,
                      output_size.x * aspect.y / (input_size.x * aspect.x),
                      overscale);
        if (force_integer_scaling.y > 0.5 && scale.y > 1.0) {
            scale.y = floor(scale.y);
        }
        scale.x = scale.y * aspect.x / aspect.y;
        if (force_integer_scaling.x > 0.5 && scale.x > 1.0) {
            scale.x = floor(scale.x);
        }
    }
    return scale;
}

vec2 transform(vec2 x, vec2 input_center, vec2 scale, vec2 output_center) {
    return (x - input_center) * scale + output_center;
}

vec2 o2i(vec2 x, vec2 input_size, vec2 output_size, vec4 crop, vec2 shift,
         uint rotation, float center_after_cropping, float force_aspect_ratio,
         vec2 aspect, vec2 force_integer_scaling, float overscale) {
    const vec2 scale_i2o = get_scale_i2o(
        input_size, output_size, crop, rotation, center_after_cropping,
        force_aspect_ratio, aspect, force_integer_scaling, overscale);
    return transform(x, vec2(0.5), output_size / scale_i2o,
                     get_input_center(input_size, output_size, scale_i2o, crop,
                                      shift, rotation, center_after_cropping));
}

vec2 i2o(vec2 x, vec2 input_size, vec2 output_size, vec4 crop, vec2 shift,
         uint rotation, float center_after_cropping, float force_aspect_ratio,
         vec2 aspect, vec2 force_integer_scaling, float overscale) {
    const vec2 scale_i2o = get_scale_i2o(
        input_size, output_size, crop, rotation, center_after_cropping,
        force_aspect_ratio, aspect, force_integer_scaling, overscale);
    return transform(x,
                     get_input_center(input_size, output_size, scale_i2o, crop,
                                      shift, rotation, center_after_cropping),
                     scale_i2o / output_size, vec2(0.5));
}

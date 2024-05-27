/*
    Input transformation library v1.2 by fishku
    Copyright (C) 2023
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
    v1.2: Rename to "input transform". Add translation option.
    v1.1: Add overscaling option. Unify parameters.
    v1.0: Initial conversion from blur_fill release. Add rotation support.
*/

vec2 get_rotated_size(vec2 x, uint rotation) {
    switch (rotation) {
        case 0:
        case 2:
        default:
            return x;
        case 1:
        case 3:
            return x.yx;
    }
}

vec4 get_rotated_crop(vec4 crop, uint rotation) {
    switch (rotation) {
        case 0:
        default:
            return crop;
        case 1:
            return crop.yzwx;
        case 2:
            return crop.zwxy;
        case 3:
            return crop.wxyz;
    }
}

vec2 get_rotated_vector(vec2 x, uint rotation) {
    switch (rotation) {
        case 0:
        default:
            return x;
        case 1:
            return vec2(-x.y, x.x);
        case 2:
            return -x;
        case 3:
            return vec2(x.y, -x.x);
    }
}

// Get 2 corners of input in texel space, spanning the input image.
// corners.x and .y define the top-left corner, corners.z and .w define the
// bottom-right corner.
vec4 get_input_corners(vec2 input_size, vec4 crop, uint rotation) {
    crop = get_rotated_crop(crop, rotation);
    return vec4(crop.y, crop.x, input_size.x - crop.w, input_size.y - crop.z);
}

// Get adjusted center in input pixel coordinate system.
vec2 get_input_center(vec2 input_size, vec4 crop, vec2 shift, uint rotation,
                      float center_after_cropping) {
    crop = get_rotated_crop(crop, rotation);
    shift = get_rotated_vector(shift, rotation);
    return (center_after_cropping > 0.5
                ? 0.5 * vec2(crop.y + input_size.x - crop.w,
                             crop.x + input_size.y - crop.z)
                : vec2(0.49999) * input_size) -
           shift;
}

// Scaling from unit output to pixel input space.
vec2 get_scale_o2i(vec2 input_size, vec2 output_size, vec4 crop, uint rotation,
                   float center_after_cropping, float force_aspect_ratio,
                   vec2 aspect, vec2 force_integer_scaling, float overscale,
                   bool output_size_is_final_viewport_size) {
    crop = get_rotated_crop(crop, rotation);
    if (output_size_is_final_viewport_size) {
        output_size = get_rotated_size(output_size, rotation);
    }
    aspect = get_rotated_size(aspect, rotation);
    // Aspect ratio before cropping.
    // lambda_1 * input_pixels.x, lambda_2 * input_pixels.y,
    // possibly corrected for forced aspect ratio
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
    float scale_x, scale_y;
    if (output_size.x / (input_size.x * aspect.x) <
        output_size.y / (input_size.y * aspect.y)) {
        // Scale will be limited by width. Calc x scale, then derive y scale
        // using aspect ratio.
        scale_x = mix(output_size.x / input_size.x,
                      output_size.y * aspect.x / (input_size.y * aspect.y),
                      overscale);
        if (force_integer_scaling.x > 0.5 && scale_x > 1.0) {
            scale_x = floor(scale_x);
        }
        scale_y = scale_x * aspect.y / aspect.x;
        if (force_integer_scaling.y > 0.5 && scale_y > 1.0) {
            scale_y = floor(scale_y);
        }
    } else {
        // Scale will be limited by height.
        scale_y = mix(output_size.y / input_size.y,
                      output_size.x * aspect.y / (input_size.x * aspect.x),
                      overscale);
        if (force_integer_scaling.y > 0.5 && scale_y > 1.0) {
            scale_y = floor(scale_y);
        }
        scale_x = scale_y * aspect.x / aspect.y;
        if (force_integer_scaling.x > 0.5 && scale_x > 1.0) {
            scale_x = floor(scale_x);
        }
    }
    return output_size / vec2(scale_x, scale_y);
}

// From unit output to pixel input space.
// coord_in_input_space = o2i(coord_in_output_space)
// This is used to sample from the input texture in the output pass.
// Version where scale is passed in.
vec2 o2i(vec2 x, vec2 input_size, vec4 crop, vec2 shift, uint rotation,
         float center_after_cropping, vec2 scale_o2i) {
    return (x - 0.49999) * scale_o2i + get_input_center(input_size, crop, shift,
                                                        rotation,
                                                        center_after_cropping);
}

// Version that computes scale.
vec2 o2i(vec2 x, vec2 input_size, vec2 output_size, vec4 crop, vec2 shift,
         uint rotation, float center_after_cropping, float force_aspect_ratio,
         vec2 aspect, vec2 force_integer_scaling, float overscale,
         bool output_size_is_final_viewport_size) {
    return o2i(x, input_size, crop, shift, rotation, center_after_cropping,
               get_scale_o2i(input_size, output_size, crop, rotation,
                             center_after_cropping, force_aspect_ratio, aspect,
                             force_integer_scaling, overscale,
                             output_size_is_final_viewport_size));
}

// From pixel input to unit output space.
// Version where scale is passed in.
vec2 i2o(vec2 x, vec2 input_size, vec4 crop, vec2 shift, uint rotation,
         float center_after_cropping, vec2 scale_o2i) {
    return (x - get_input_center(input_size, crop, shift, rotation,
                                 center_after_cropping)) /
               scale_o2i +
           0.49999;
}

// Version that computes scale.
vec2 i2o(vec2 x, vec2 input_size, vec2 output_size, vec4 crop, vec2 shift,
         uint rotation, float center_after_cropping, float force_aspect_ratio,
         vec2 aspect, vec2 force_integer_scaling, float overscale,
         bool output_size_is_final_viewport_size) {
    return i2o(x, input_size, crop, shift, rotation, center_after_cropping,
               get_scale_o2i(input_size, output_size, crop, rotation,
                             center_after_cropping, force_aspect_ratio, aspect,
                             force_integer_scaling, overscale,
                             output_size_is_final_viewport_size));
}

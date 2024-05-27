#version 450

// See compose.slang for copyright and other information.

// Crop is rotated already.
vec4 get_effective_corners(vec4 crop, vec4 input_size, float sample_size) {
    return input_size.zwzw * vec4(crop.y,                               //
                                  input_size.y - crop.z - sample_size,  //
                                  input_size.x - crop.w,                //
                                  input_size.y - crop.z);
}

#include "crop_and_sample_common.inc"

#version 450

// See compose.slang for copyright and other information.

vec4 get_effective_corners(vec4 crop, vec4 input_size, float sample_size) {
    return input_size.zwzw * vec4(crop.y,                 //
                                  crop.x,                 //
                                  input_size.x - crop.w,  //
                                  crop.x + sample_size);
}

#include "crop_and_sample_common.inc"

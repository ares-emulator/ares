#version 450

// See compose.slang for copyright and other information.

#include "../../../misc/shaders/input_transform/input_transform.inc"
#include "parameters.inc"

layout(push_constant) uniform Push {
    vec4 InputSize;
    vec4 FinalViewportSize;
    uint Rotation;
    // Own settings
    float EXTEND_H;
    float EXTEND_V;
    float MIRROR_BLUR;
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
}
param;

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; }
global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 tx_coord;
layout(location = 2) out vec4 input_corners;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    const vec4 crop = vec4(param.OS_CROP_TOP, param.OS_CROP_LEFT,
                           param.OS_CROP_BOTTOM, param.OS_CROP_RIGHT);
    // Because RA "rotates" the final viewport size on rotated cores, we need to
    // undo that rotation so that the math checks out. The 0 and 180 degree
    // rotations and the 90 and 270 rotations are symmetric for 2D sizes, which
    // is why we can use param.Rotation directly here.
    const vec2 final_viewport_size_rotated =
        get_rotated_size(param.FinalViewportSize.xy, param.Rotation);
    const vec2 scale_i2o = get_scale_i2o(
        param.InputSize.xy, final_viewport_size_rotated, crop, param.Rotation,
        param.CENTER_AFTER_CROPPING, param.FORCE_ASPECT_RATIO,
        vec2(param.ASPECT_H, param.ASPECT_V),
        vec2(param.FORCE_INTEGER_SCALING_H, param.FORCE_INTEGER_SCALING_V),
        param.OVERSCALE);
    const vec2 shift = vec2(param.SHIFT_H, param.SHIFT_V);
    const vec2 input_center = get_input_center(
        param.InputSize.xy, final_viewport_size_rotated, scale_i2o, crop, shift,
        param.Rotation, param.CENTER_AFTER_CROPPING);
    tx_coord = transform(TexCoord, vec2(0.5),
                         final_viewport_size_rotated / scale_i2o, input_center);
    input_corners = get_input_corners(param.InputSize.xy, crop, param.Rotation);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 tx_coord;
layout(location = 2) in vec4 input_corners;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

float min_of_vec4(vec4 i) { return min(min(i.x, i.y), min(i.z, i.w)); }

int argmin(vec4 i) {
    const vec4 m = vec4(min_of_vec4(i));
    const vec4 ma = step(i, m) * vec4(0.0, -1.0, -2.0, -3.0);
    return -int(min_of_vec4(ma));
}

// Returns a coordinate in [0, w) when repeating that interval (and optionally
// mirroring).
float mirrored_repeat(float w, float x) {
    const float phase = mod(x, w);
    if (param.MIRROR_BLUR < 0.5) {
        return phase;
    }
    const int period = int(x / w);
    return period % 2 == 1 != x > 0.0 ? phase : w - phase;
}

float extend_left(vec2 coord, vec4 input_corners) {
    return input_corners.x +
           mirrored_repeat(param.SAMPLE_SIZE, coord.x - input_corners.x);
}

float extend_right(vec2 coord, vec4 input_corners) {
    return input_corners.z -
           mirrored_repeat(param.SAMPLE_SIZE, input_corners.z - coord.x);
}

float extend_top(vec2 coord, vec4 input_corners) {
    return input_corners.y +
           mirrored_repeat(param.SAMPLE_SIZE, coord.y - input_corners.y);
}

float extend_bottom(vec2 coord, vec4 input_corners) {
    return input_corners.w -
           mirrored_repeat(param.SAMPLE_SIZE, input_corners.w - coord.y);
}

// This function samples in a very specific way which is the foundation for
// blurring later.
// - If the sample coordinate is outside of the cropped input, Either black is
// returned if blur extension is turned off, or a repeated pattern from the
// sampling frame band is returned.
// - If the coordinate is inside the cropped input and within the frame band
// given by SAMPLE_SIZE, the original texture sample is returned.
// - If the coordinate is further inside than the frame band, a mirrored
// repeating sample is returned. The side of the frame that is sampled is given
// by the one that is closest to the sampled point.
vec3 sample_mirrored_frame(sampler2D tex, vec2 tx_coord, vec4 input_corners) {
    const vec2 extend_fill =
        get_rotated_size(vec2(param.EXTEND_H, param.EXTEND_V), param.Rotation);
    if (tx_coord.x < input_corners.x) {
        if (extend_fill.x < 0.5) {
            return vec3(0.0);
        }
        if (tx_coord.y < input_corners.y) {
            // Top left corner extension
            if (extend_fill.y < 0.5) {
                return vec3(0.0);
            }
            return texture(tex, vec2(extend_left(tx_coord, input_corners),
                                     extend_top(tx_coord, input_corners)) *
                                    param.InputSize.zw)
                .rgb;
        } else if (tx_coord.y < input_corners.w) {
            // Left extension
            return texture(tex, vec2(extend_left(tx_coord, input_corners),
                                     tx_coord.y) *
                                    param.InputSize.zw)
                .rgb;
        } else {
            // Bottom left corner extension
            if (extend_fill.y < 0.5) {
                return vec3(0.0);
            }
            return texture(tex, vec2(extend_left(tx_coord, input_corners),
                                     extend_bottom(tx_coord, input_corners)) *
                                    param.InputSize.zw)
                .rgb;
        }
    } else if (tx_coord.x < input_corners.z) {
        if (tx_coord.y < input_corners.y) {
            if (extend_fill.y < 0.5) {
                return vec3(0.0);
            }
            // Top extension
            return texture(tex, vec2(tx_coord.x,
                                     extend_top(tx_coord, input_corners)) *
                                    param.InputSize.zw)
                .rgb;
        } else if (tx_coord.y < input_corners.w) {
            const vec4 inner_corners =
                input_corners + vec4(param.SAMPLE_SIZE, param.SAMPLE_SIZE,
                                     -param.SAMPLE_SIZE, -param.SAMPLE_SIZE);
            if (any(lessThan(tx_coord, inner_corners.xy)) ||
                any(greaterThanEqual(tx_coord, inner_corners.zw))) {
                // In frame band
                return texture(tex, tx_coord * param.InputSize.zw).rgb;
            }
            // Innermost -- mirrored repeat sampling from nearest side
            const vec4 distances = vec4(
                tx_coord.x - inner_corners.x, inner_corners.z - tx_coord.x,
                tx_coord.y - inner_corners.y, inner_corners.w - tx_coord.y);
            switch (argmin(distances)) {
                case 0:
                    // left
                    return texture(tex,
                                   vec2(extend_left(tx_coord, input_corners),
                                        tx_coord.y) *
                                       param.InputSize.zw)
                        .rgb;
                case 1:
                    // right
                    return texture(tex,
                                   vec2(extend_right(tx_coord, input_corners),
                                        tx_coord.y) *
                                       param.InputSize.zw)
                        .rgb;
                case 2:
                    // top
                    return texture(tex,
                                   vec2(tx_coord.x,
                                        extend_top(tx_coord, input_corners)) *
                                       param.InputSize.zw)
                        .rgb;
                case 3:
                default:
                    // bottom
                    return texture(tex, vec2(tx_coord.x,
                                             extend_bottom(tx_coord,
                                                           input_corners)) *
                                            param.InputSize.zw)
                        .rgb;
            }
        } else {
            if (extend_fill.y < 0.5) {
                return vec3(0.0);
            }
            // Bottom extension
            return texture(tex, vec2(tx_coord.x,
                                     extend_bottom(tx_coord, input_corners)) *
                                    param.InputSize.zw)
                .rgb;
        }
    } else {
        if (extend_fill.x < 0.5) {
            return vec3(0.0);
        }
        if (tx_coord.y < input_corners.y) {
            // Top right corner extension
            if (extend_fill.y < 0.5) {
                return vec3(0.0);
            }
            return texture(tex, vec2(extend_right(tx_coord, input_corners),
                                     extend_top(tx_coord, input_corners)) *
                                    param.InputSize.zw)
                .rgb;
        } else if (tx_coord.y < input_corners.w) {
            // Right extension
            return texture(tex, vec2(extend_right(tx_coord, input_corners),
                                     tx_coord.y) *
                                    param.InputSize.zw)
                .rgb;
        } else {
            // Bottom right corner extension
            if (extend_fill.y < 0.5) {
                return vec3(0.0);
            }
            return texture(tex, vec2(extend_right(tx_coord, input_corners),
                                     extend_bottom(tx_coord, input_corners)) *
                                    param.InputSize.zw)
                .rgb;
        }
    }
}

void main() {
    FragColor =
        vec4(sample_mirrored_frame(Source, tx_coord, input_corners), 1.0);
}

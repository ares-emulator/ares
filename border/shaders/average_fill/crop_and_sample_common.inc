// See compose.slang for copyright and other information.

#include "../../../misc/shaders/input_transform/input_transform.inc"
#include "parameters.inc"

layout(push_constant) uniform Push {
    vec4 InputSize;
    uint Rotation;
    // Own settings
    float SAMPLE_SIZE;
    // From input transform library, cropping section
    float OS_CROP_TOP;
    float OS_CROP_BOTTOM;
    float OS_CROP_LEFT;
    float OS_CROP_RIGHT;
}
param;

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; }
global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main() {
    gl_Position = global.MVP * Position;
    const vec4 rotated_crop =
        get_rotated_crop(vec4(param.OS_CROP_TOP, param.OS_CROP_LEFT,
                              param.OS_CROP_BOTTOM, param.OS_CROP_RIGHT),
                         param.Rotation);
    const vec4 effective_corners =
        get_effective_corners(rotated_crop, param.InputSize, param.SAMPLE_SIZE);
    vTexCoord = mix(effective_corners.xy, effective_corners.zw, TexCoord);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Input;

void main() { FragColor = vec4(texture(Input, vTexCoord).rgb, 1.0); }

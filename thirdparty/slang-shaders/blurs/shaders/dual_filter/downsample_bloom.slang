#version 450

// See dual_filter.inc for copyright and other information.

#include "dual_filter.inc"
#include "parameters.inc"
#include "parameters_bloom.inc"

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float BLUR_RADIUS;
    float BLOOM_THRESHOLD;
}
param;

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 MVP;
}
global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main() {
    // Use the output pixel size as reference here to be more robust against different bloom
    // configurations.
    const vec2 offset = 0.5 * param.OutputSize.zw * param.BLUR_RADIUS;
    FragColor = vec4(pow(downsample(Source, vTexCoord, offset), vec3(param.BLOOM_THRESHOLD)), 1.0);
}

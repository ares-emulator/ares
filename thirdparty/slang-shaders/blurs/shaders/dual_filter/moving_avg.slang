#version 450

// See dual_filter.inc for copyright and other information.

#include "parameters.inc"
#include "parameters_bloom.inc"

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float ADAPT_SPEED;
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
layout(set = 0, binding = 3) uniform sampler2D MovingAverageFeedback;

void main() {
    FragColor = vec4(mix(texture(MovingAverageFeedback, vTexCoord).rgb,
                         textureLod(Source, vTexCoord, 9000.1).rgb, param.ADAPT_SPEED),
                     1.0);
}

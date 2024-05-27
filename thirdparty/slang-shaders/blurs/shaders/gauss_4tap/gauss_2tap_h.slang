#version 450

// See common.inc for copyright and other information.

#include "parameters.inc"

layout(push_constant) uniform Push {
    vec4 SourceSize;
    float SIGMA;
}
param;

#include "common.inc"

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; }
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
    const vec2 offset = vec2(get_offset(param.SIGMA) * param.SourceSize.z, 0.0);
    FragColor = 0.5 * (texture(Source, vTexCoord - offset) +
                       texture(Source, vTexCoord + offset));
}

#version 450
// SPDX-License-Identifier: Unlicense
#pragma name SMAA_Pass2
//-----------------------------------------------------------------------------
// Neighborhood Blending Shader (Third Pass)

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 MVP;
} global;

#define SMAA_RT_METRICS vec4(params.SourceSize.z, params.SourceSize.w, params.SourceSize.x, params.SourceSize.y)
#define SMAA_GLSL_4

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 offset;

#define SMAA_INCLUDE_PS 0
#include "SMAA.hlsl"

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    SMAANeighborhoodBlendingVS(TexCoord, offset);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 offset;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D SMAA_Input;

#define SMAA_INCLUDE_VS 0
#include "SMAA.hlsl"

void main() {
    FragColor = SMAANeighborhoodBlendingPS(vTexCoord, offset, SMAA_Input, Source);
}

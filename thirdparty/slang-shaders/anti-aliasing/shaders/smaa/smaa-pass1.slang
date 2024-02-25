#version 450
// SPDX-License-Identifier: Unlicense
#pragma name SMAA_Pass1
//-----------------------------------------------------------------------------
// Blending Weight Calculation Shader (Second Pass)

#pragma parameter SMAA_THRESHOLD "SMAA Threshold" 0.05 0.01 0.5 0.01
#pragma parameter SMAA_MAX_SEARCH_STEPS "SMAA Max Search Steps" 32.0 4.0 112.0 1.0
#pragma parameter SMAA_MAX_SEARCH_STEPS_DIAG "SMAA Max Search Steps Diagonal" 16.0 4.0 20.0 1.0
#pragma parameter SMAA_CORNER_ROUNDING "SMAA Corner Rounding" 25.0 0.0 100.0 1.0

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float SMAA_THRESHOLD;
    float SMAA_MAX_SEARCH_STEPS;
    float SMAA_MAX_SEARCH_STEPS_DIAG;
    float SMAA_CORNER_ROUNDING;
} params;

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 MVP;
} global;

#define SMAA_RT_METRICS vec4(params.SourceSize.z, params.SourceSize.w, params.SourceSize.x, params.SourceSize.y)
#define SMAA_GLSL_4

float THRESHOLD = params.SMAA_THRESHOLD;
float MAX_SEARCH_STEPS = params.SMAA_MAX_SEARCH_STEPS;
float MAX_SEARCH_STEPS_DIAG = params.SMAA_MAX_SEARCH_STEPS_DIAG;
float CORNER_ROUNDING = params.SMAA_CORNER_ROUNDING;
#define SMAA_THRESHOLD THRESHOLD
#define SMAA_MAX_SEARCH_STEPS MAX_SEARCH_STEPS
#define SMAA_MAX_SEARCH_STEPS_DIAG MAX_SEARCH_STEPS_DIAG
#define SMAA_CORNER_ROUNDING CORNER_ROUNDING

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 pixcoord;
layout(location = 2) out vec4 offset[3];

#define SMAA_INCLUDE_PS 0
#include "SMAA.hlsl"

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    SMAABlendingWeightCalculationVS(TexCoord, pixcoord, offset);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 pixcoord;
layout(location = 2) in vec4 offset[3];
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D areaTex;
layout(set = 0, binding = 4) uniform sampler2D searchTex;

#define SMAA_INCLUDE_VS 0
#include "SMAA.hlsl"

void main() {
    vec4 subsampleIndices = vec4(0.0);
    FragColor = SMAABlendingWeightCalculationPS(vTexCoord, pixcoord, offset, Source, areaTex, searchTex, subsampleIndices);
}

#version 450
// SPDX-License-Identifier: Unlicense
#pragma name SMAA_Pass0
//-----------------------------------------------------------------------------
// Edge Detection Shaders (First Pass)

#pragma parameter SMAA_EDT "SMAA Edge Detection: Luma | Color" 1.0 0.0 1.0 1.0
#pragma parameter SMAA_THRESHOLD "SMAA Threshold" 0.05 0.01 0.5 0.01
#pragma parameter SMAA_MAX_SEARCH_STEPS "SMAA Max Search Steps" 32.0 4.0 112.0 1.0
#pragma parameter SMAA_MAX_SEARCH_STEPS_DIAG "SMAA Max Search Steps Diagonal" 16.0 4.0 20.0 1.0
#pragma parameter SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR "SMAA Local Contrast Adapt. Factor" 2.0 1.0 4.0 0.1

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float SMAA_EDT;
    float SMAA_THRESHOLD;
    float SMAA_MAX_SEARCH_STEPS;
    float SMAA_MAX_SEARCH_STEPS_DIAG;
    float SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR;
} params;

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 MVP;
} global;

#define SMAA_RT_METRICS vec4(params.SourceSize.z, params.SourceSize.w, params.SourceSize.x, params.SourceSize.y)
#define SMAA_GLSL_4

float THRESHOLD = params.SMAA_THRESHOLD;
float MAX_SEARCH_STEPS = params.SMAA_MAX_SEARCH_STEPS;
float MAX_SEARCH_STEPS_DIAG = params.SMAA_MAX_SEARCH_STEPS_DIAG;
float LOCAL_CONTRAST_ADAPTATION_FACTOR = params.SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR;
#define SMAA_THRESHOLD THRESHOLD
#define SMAA_MAX_SEARCH_STEPS MAX_SEARCH_STEPS
#define SMAA_MAX_SEARCH_STEPS_DIAG MAX_SEARCH_STEPS_DIAG
#define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR LOCAL_CONTRAST_ADAPTATION_FACTOR

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 offset[3];

#define SMAA_INCLUDE_PS 0
#include "SMAA.hlsl"

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    SMAAEdgeDetectionVS(TexCoord, offset);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 offset[3];
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define SMAA_INCLUDE_VS 0
#include "SMAA.hlsl"

void main() {
    if (params.SMAA_EDT == 0.0) {
        FragColor = vec4(SMAALumaEdgeDetectionPS(vTexCoord, offset, Source), 0.0, 0.0);
    } else if (params.SMAA_EDT <= 1.0) {
        FragColor = vec4(SMAAColorEdgeDetectionPS(vTexCoord, offset, Source), 0.0, 0.0);
    // Unavailable as we don't have access to a depth buffer (yet?)
    // } else if (params.SMAA_EDT <= 2.0) {
    //     FragColor = vec4(SMAADepthEdgeDetectionPS(vTexCoord, offset, depthTex), 0.0, 0.0);
    }
}

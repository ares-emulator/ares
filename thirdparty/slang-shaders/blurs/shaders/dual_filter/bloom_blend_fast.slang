#version 450

// See dual_filter.inc for copyright and other information.

#include "dual_filter.inc"
#include "parameters.inc"
#include "parameters_bloom.inc"
#include "tonemapping.inc"

#pragma parameter SCALE_EMPH "Bloom scale emphasis" 0.0 0.0 3.0 0.1
#pragma parameter SCALE_SPREAD "Bloom scale spread" 4.0 0.5 10.0 0.5

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    vec4 InputSize;
    uint FrameCount;
    float MIN_EXP_INTENSITY;
    float EYE_ADAPTION;
    float MIN_EXP;
    float MAX_EXP;
    float SCALE_EMPH;
    float SCALE_SPREAD;
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
layout(set = 0, binding = 3) uniform sampler2D Input;
layout(set = 0, binding = 4) uniform sampler2D MovingAverage;
layout(set = 0, binding = 5) uniform sampler2D Scale1;
layout(set = 0, binding = 6) uniform sampler2D Scale2;
layout(set = 0, binding = 7) uniform sampler2D Scale3;
layout(set = 0, binding = 8) uniform sampler2D Scale4;

float scale_gauss(float x) {
    const float dx = x - param.SCALE_EMPH;
    return exp(-dx * dx / (2.0 * param.SCALE_SPREAD * param.SCALE_SPREAD));
}

void main() {
    // Weigh the different scale levels.
    const float w1 = scale_gauss(0.0);
    const float w2 = scale_gauss(1.0);
    const float w3 = scale_gauss(2.0);
    const float w4 = scale_gauss(3.0);
    vec3 bloom = w1 * texture(Scale1, vTexCoord).rgb +  //
                 w2 * texture(Scale2, vTexCoord).rgb +  //
                 w3 * texture(Scale3, vTexCoord).rgb +  //
                 w4 * texture(Scale4, vTexCoord).rgb / (w1 + w2 + w3 + w4);

    // Moving average luminance sampled from respective pass.
    const vec3 ambient = texture(MovingAverage, vec2(0.5)).rgb;
    bloom =
        tonemap(bloom * ambient_to_intensity(ambient, param.MIN_EXP_INTENSITY,
                                             param.MIN_EXP_INTENSITY * (1.0 - param.EYE_ADAPTION),
                                             param.MIN_EXP, param.MAX_EXP));

    const vec2 sharp_coord = (floor(vTexCoord * param.InputSize.xy) + 0.5) * param.InputSize.zw;
    FragColor = vec4(blend_screen(bloom, texture(Input, sharp_coord).rgb), 1.0);
}

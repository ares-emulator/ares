#version 450

// See dual_filter.inc for copyright and other information.

#include "dual_filter.inc"
#include "parameters.inc"
#include "parameters_bloom.inc"
#include "tonemapping.inc"

layout(push_constant) uniform Push {
    vec4 InputSize;
    vec4 Downsample1Size;
    vec4 Downsample2Size;
    vec4 UpsampleSize;
    float BLUR_RADIUS;
    float MIN_EXP_INTENSITY;
    float EYE_ADAPTION;
    float MIN_EXP;
    float MAX_EXP;
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
layout(set = 0, binding = 4) uniform sampler2D Downsample1;
layout(set = 0, binding = 5) uniform sampler2D Downsample2;
layout(set = 0, binding = 6) uniform sampler2D Upsample;
layout(set = 0, binding = 7) uniform sampler2D MovingAverage;

void main() {
    const vec3 ambient = texture(MovingAverage, vec2(0.5)).rgb;
    // Hack: "Fastest" bloom appears relatively less intense compared to the regular preset.
    // Using an unnormalized value achieves about equal intensity.
    vec3 bloom =
        3.0 * upsample(Downsample1, vTexCoord, 0.5 * param.Downsample1Size.zw * param.BLUR_RADIUS) +
        1.0 * upsample(Downsample2, vTexCoord, 0.5 * param.Downsample2Size.zw * param.BLUR_RADIUS) +
        3.0 * upsample(Upsample, vTexCoord, 0.5 * param.UpsampleSize.zw * param.BLUR_RADIUS);
    bloom =
        tonemap(bloom * ambient_to_intensity(ambient, param.MIN_EXP_INTENSITY,
                                             param.MIN_EXP_INTENSITY * (1.0 - param.EYE_ADAPTION),
                                             param.MIN_EXP, param.MAX_EXP));

    const vec2 sharp_coord = (floor(vTexCoord * param.InputSize.xy) + 0.5) * param.InputSize.zw;
    FragColor = vec4(blend_screen(bloom, texture(Input, sharp_coord).rgb), 1.0);
}

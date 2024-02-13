#version 450

// This is one of several passes needed to cheaply emulate the bloom effect.

#include "config.inc"
#include "includes/functions.include.slang"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vFuzzy_main_pass_stage_1;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;

    //Get fuzzy mul and pow factor
        vFuzzy_main_pass_stage_1 = apply_fuzzy_main_pass_stage_1();
}

#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vFuzzy_main_pass_stage_1;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D colortools_and_ntsc_pass;

//#include "includes/functions.include.slang"
#include "includes/blooms.include.slang"

void main() {
    if (DO_BLOOM == 0.0) return;
    
    vec3 bloomed = bloom_gamma_power(
        colortools_and_ntsc_pass,
        vTexCoord,
        params.OriginalSize,
        vec2(BLOOM_SIZE),
        BLOOM_QUALITY,
        BLOOM_GAMMA, 1.0,
        0.0,
        0.0
    );
    bloomed = apply_fuzzy_main_pass_stage_2(bloomed, vFuzzy_main_pass_stage_1);
    FragColor = vec4(bloomed, 1.0);
}

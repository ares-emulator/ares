#version 450
#include "config.inc"

#define eps 1e-8

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vOutputCoord;

#include "functions.include"
void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = get_scaled_coords(TexCoord);
    vOutputCoord = TexCoord;
}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vOutputCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D final_pass;
layout(set = 0, binding = 4) uniform sampler2D ambi_temporal_pass;

#include "functions.include"

vec4 pixel_border() {
    vec3 ambi = texture(ambi_temporal_pass, vOutputCoord).rgb;
    float l = length(ambi);
    float sat = 1.25;
    float bright = 1.25;
    ambi =  normalize( pow(ambi.rgb + vec3(eps), vec3(sat)))*l * bright  ;
    ambi*=bright;
    return vec4(ambi,0.0);
}

vec3 ambi_noised() {
    return pixel_border().rgb + random();
}


void main() {
    vec4 psample = texture(final_pass, vOutputCoord);
    if (DO_AMBILIGHT == 1) {
        vec3 ambinoised = ambi_noised();
        if (is_outer_frame(psample)) {
            FragColor = mark_outer_frame(ambi_noised());
        } else {
            FragColor = vec4(mix(ambinoised.rgb,psample.rgb,min(psample.a*1.5,1.0)),psample.a);
        }
    } else {
        FragColor = psample ;
    }


    //FragColor = vec4(ambi_noised(), 1.0);
    
}


#version 450
#include "config.inc"
#define pi 3.141592654

// Blur sizes must not depend on input resolution
#define scalemod_x (global.in_glow_passSize.x/360.0)
#define scalemod_y (global.in_glow_passSize.y/270.0)


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vOutputCoord;


void main() {
   gl_Position = global.MVP * Position;
   //vTexCoord = get_scaled_coords(TexCoord);
   vTexCoord = TexCoord;
   vOutputCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vOutputCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D FXAA_pass;
layout(set = 0, binding = 3) uniform sampler2D first_pass;
layout(set = 0, binding = 4) uniform sampler2D in_glow_pass;
layout(set = 0, binding = 5) uniform sampler2D shift_and_bleed_pass;

#include "includes/pixel_glows.include.slang"    


vec4 main_wrap(void) {
    //Halo
    vec3 pixel_haloed;
    vec2 halo_coords = vTexCoord;

    if (DO_IN_GLOW == 1.0)
        pixel_haloed = pixel_glow(in_glow_pass, HALO_W, HALO_H, HALO_POWER, HALO_GAMMA, halo_coords, global.FXAA_passSize, global.FXAA_passSize).rgb;
    else if ( DO_SHIFT_RGB + DO_SAT_BLEED > 0.0)
        pixel_haloed = pixel_glow(shift_and_bleed_pass, HALO_W, HALO_H, HALO_POWER, HALO_GAMMA, halo_coords, global.FXAA_passSize, global.FXAA_passSize).rgb;
    else if (DO_FXAA > 0.0)
        pixel_haloed = pixel_glow(FXAA_pass, HALO_W, HALO_H, HALO_POWER, HALO_GAMMA, halo_coords, global.FXAA_passSize, global.FXAA_passSize).rgb;
    else
        pixel_haloed = pixel_glow(first_pass, HALO_W, HALO_H, HALO_POWER, HALO_GAMMA, halo_coords, global.FXAA_passSize, global.FXAA_passSize).rgb;
        
    return vec4(pixel_haloed.rgb,1.0);
}



void main() {
    if (DO_HALO == 0.0 ) return;
    
    FragColor = main_wrap();
}

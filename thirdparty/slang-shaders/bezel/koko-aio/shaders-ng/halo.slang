#version 450
#include "config.inc"
#include "includes/functions.include.slang"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vHALO_SHARPNESS;
layout(location = 2) out float vHALO_POWER;
layout(location = 3) out float vDeltaRenderOk;

vec2 radius_blur_adapt(vec2 bsize, vec4 sourcesize){
    vec2 r;
    r.x = bsize.x / SCALEMOD_X;
    r.y = r.x *  get_in_aspect();
    
	float aspect = (sourcesize.x/sourcesize.y);
    
	r = r / vec2(1.0, aspect);

    return r;
}

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;

    //Adapt to the old params range
        vHALO_SHARPNESS = vec2(GLOW_SHARP_MAX - HALO_SHARPNESS);
        vHALO_SHARPNESS = vHALO_SHARPNESS/6.0;


//20..1 10...2 prova 5..3
    //Correct aspect
        vHALO_SHARPNESS = radius_blur_adapt(vHALO_SHARPNESS, params.SourceSize);


   if (HALO_POWER >= 0.0)
      vHALO_POWER = HALO_POWER;
         else
      vHALO_POWER = -HALO_POWER/10.0;
    
    bool bIs_Interlaced = is_interlaced();
    vDeltaRenderOk = is_deltarender_allowed(bIs_Interlaced); 
    
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vHALO_SHARPNESS;
layout(location = 2) in float vHALO_POWER;
layout(location = 3) in float vDeltaRenderOk;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 4) uniform sampler2D Source;
layout(set = 0, binding = 5) uniform sampler2D in_glow_pass_x;

void main() {
    
    if (DO_HALO == 0.0 ) return;
    
    //Pass the right texture unchanged for tighter blurs:
    if (HALO_SHARPNESS >= GLOW_SHARP_MAX) {
        FragColor = vec4( pow(texture(Source, vTexCoord).rgb * vHALO_POWER,  vec3(HALO_GAMMA_OUT)), 0.0) ;
        return;
    }
    
    //pixel_haloed = blur5_x(Source, vTexCoord, params.SourceSize.xy, vHALO_SHARPNESS, 0.0);
    vec3 pixel_haloed = blur_px(Source, vTexCoord, vHALO_SHARPNESS) * vHALO_POWER;

    pixel_haloed =  pow(pixel_haloed, vec3(HALO_GAMMA_OUT));
    
    FragColor = vec4(pixel_haloed.rgb, 0.0);
    
}

#version 450
#include "config.inc"
#include "includes/functions.include.slang" 

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 2) out vec3 vHALO_GAMMA;
layout(location = 3) out float vHALO_UNGAIN_FACTOR;
layout(location = 4) out float vDo_pixelgrid_h;
layout(location = 5) out float vTexCoord_mul_sourcesizey;
layout(location = 6) out float vScanline_gap;
layout(location = 7) out float vDeltaRenderOk;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;

    //Shift texel coord to optimize blurring.
    //will invert the shift in the second pass.
        float m = map_range(HALO_SHARPNESS, 0.0, GLOW_SHARP_MAX, 0.0, 1.0);
        vTexCoord -= mix( params.SourceSize.zw*0.5, vec2(0.0), m);
    
    
    vDo_pixelgrid_h = float(pixelgrid_h_needed());
    vTexCoord_mul_sourcesizey = vTexCoord.y * global.in_glow_passSize.y;
    
    vHALO_GAMMA = vec3(HALO_GAMMA);
    vHALO_UNGAIN_FACTOR = get_halo_ungain_factor();
    
    bool bIs_Interlaced = is_interlaced();
    vDeltaRenderOk = is_deltarender_allowed(bIs_Interlaced); 
    
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 2) in vec3 vHALO_GAMMA;
layout(location = 3) in float vHALO_UNGAIN_FACTOR;
layout(location = 4) in float vDo_pixelgrid_h;
layout(location = 5) in float vTexCoord_mul_sourcesizey;
layout(location = 6) in float vScanline_gap;
layout(location = 7) in float vDeltaRenderOk;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 4) uniform sampler2D in_glow_pass;

void main() {
    if (DO_HALO == 0.0 ) return;
        
    vec3 pixel_haloed = texture(in_glow_pass, vTexCoord).rgb;

    if (HALO_NO_PREGAIN == 1.0)  pixel_haloed/= vHALO_UNGAIN_FACTOR;
    
    pixel_haloed = pow(pixel_haloed, vHALO_GAMMA);
    
    FragColor.rgb = pixel_haloed;

}

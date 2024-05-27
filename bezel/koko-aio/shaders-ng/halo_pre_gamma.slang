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
    
    if (HALO_PRE_SCANLINE_GAP == 1.0) {
        vScanline_gap = 1-DO_PIXELGRID_H;
        vScanline_gap = mix( vScanline_gap, 1.0, HALO_VS_SCAN);
        vScanline_gap = clamp(vScanline_gap, 0.0,1.0);
    } 
                    
    
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
    
    
    vec4 pixel_haloed = texture(in_glow_pass, vTexCoord);
    
/*    if (DELTA_RENDER == 1.0) {
         if (vDeltaRenderOk == 1.0) {
             //No need to process halo if we're using deltarender and content did not change.
             //Produces glitches; disable by now.
             if (pixel_haloed.a > 3.0)
                 return;
         }
    }
*/  
    if (HALO_NO_PREGAIN == 1.0)  pixel_haloed.rgb/= vHALO_UNGAIN_FACTOR;
    
    pixel_haloed.rgb = pow(pixel_haloed.rgb, vHALO_GAMMA);
    
    //Prepaint scanlines gaps?
    if (HALO_PRE_SCANLINE_GAP == 1.0) {
        if (vDo_pixelgrid_h == 1.0) {
            if (mod( vTexCoord_mul_sourcesizey, 2.0) < 1.0) {
                //Scanline gap fades away for high luminosity pixels
                pixel_haloed.rgb = pixel_haloed.rgb * vScanline_gap;
            }
        }
    }
    FragColor = vec4(pixel_haloed.rgb, 1.0);

}

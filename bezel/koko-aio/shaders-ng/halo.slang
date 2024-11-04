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
layout(location = 4) out float vMask_helper_max_power;
layout(location = 5) out float vMask_helper_gamma_compensation;
layout(location = 6) out float vMask_helper_gain;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    //Shift texel coord to optimize blurring.
    //inverted the previous pass shift.
        float m = map_range(HALO_SHARPNESS, 0.0, GLOW_SHARP_MAX, 0.0, 1.0);
        vTexCoord += mix( params.SourceSize.zw*0.5, vec2(0.0), m);
        
    //Adapt to the old params range
        vHALO_SHARPNESS = vec2(GLOW_SHARP_MAX - HALO_SHARPNESS);
        vHALO_SHARPNESS = vHALO_SHARPNESS/7.5;

            
            
    //Correct aspect
        vHALO_SHARPNESS = radius_blur_adapt(vHALO_SHARPNESS);

   if (HALO_POWER >= 0.0)
      vHALO_POWER = HALO_POWER;
         else
      vHALO_POWER = -HALO_POWER/10.0;
    
    bool bIs_Interlaced = is_interlaced();
    vDeltaRenderOk = is_deltarender_allowed(bIs_Interlaced); 
    
   
    //Horizontal mask helper variables:
        //Lower strength if the mask saturated too late and have no visible room to reach full bright.
        //(Use a gray gamma ramp to debug)
        vMask_helper_max_power = smoothstep_fast( 1.0, 3.0, IN_GLOW_POWER ) * HALO_DO_MASK_HELPER;   
    
        vMask_helper_max_power *= DO_PIXELGRID_W;
        
        //Tune mask helper compensation gamma:
        vMask_helper_gain = 1.0;
        float out_monitor_gamma = 2.2; //A well calibrated monitor should be 2.2
        vMask_helper_gamma_compensation = out_monitor_gamma;
        if (DO_CCORRECTION == 1.0) {
            if (DO_PIXELGRID == 1.0) {
                //Scanlines alters visible output gamma
                vMask_helper_gamma_compensation = mix(out_monitor_gamma, out_monitor_gamma - 0.6, DO_PIXELGRID_H);
                
                //Vertical mask alters visible output gamma (contrast).
                vMask_helper_gamma_compensation = mix(vMask_helper_gamma_compensation, vMask_helper_gamma_compensation - 0.6, PIXELGRID_Y_MASK);
                vMask_helper_gamma_compensation = mix(vMask_helper_gamma_compensation, vMask_helper_gamma_compensation + 0.6, 
                                                    PIXELGRID_Y_MASK_ON_WHITE * PIXELGRID_Y_MASK);
            }
            //GAMMA_OUT out alters visible output gamma:
            vMask_helper_gamma_compensation = (1.0/vMask_helper_gamma_compensation) / GAMMA_OUT ;  
            
            vMask_helper_gain = max(IN_GLOW_POWER, 1.0);
        }

        vMask_helper_gain = vMask_helper_gain + eps - 1;
        
   
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vHALO_SHARPNESS;
layout(location = 2) in float vHALO_POWER;
layout(location = 3) in float vDeltaRenderOk;
layout(location = 4) in float vMask_helper_max_power;
layout(location = 5) in float vMask_helper_gamma_compensation;
layout(location = 6) in float vMask_helper_gain;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 4) uniform sampler2D Source;
layout(set = 0, binding = 5) uniform sampler2D in_glow_pass;


vec3 get_mask_helper() {
    vec3 mask_helper_pixel_in = texture(in_glow_pass, vTexCoord).rgb;

    vec3 mask_helper = (mask_helper_pixel_in.rgb - 1) / vMask_helper_gain ;
    mask_helper = clamp(mask_helper, vec3(0.0), vec3(300.0));
    mask_helper = pow( mask_helper, vec3(vMask_helper_gamma_compensation) );
    
    return mask_helper * vMask_helper_max_power;
}


void main() {
    
    if (DO_HALO == 0.0 ) return;
    
    vec3 mask_helper = vec3(0.0);
    
    if (HALO_DO_MASK_HELPER > 0.00)
        mask_helper = get_mask_helper();
    
    //Pass the right texture unchanged for tighter blurs:
    if (HALO_SHARPNESS >= GLOW_SHARP_MAX) {
        FragColor.rgb = mask_helper + pow(texture(Source, vTexCoord).rgb * vHALO_POWER,  vec3(HALO_GAMMA_OUT));
        return;
    }
    
    //pixel_haloed = blur5_x(Source, vTexCoord, params.SourceSize.xy, vHALO_SHARPNESS, 0.0);
    vec3 pixel_haloed = blur_px(Source, vTexCoord, vHALO_SHARPNESS) * vHALO_POWER;
    pixel_haloed =  pow(pixel_haloed, vec3(HALO_GAMMA_OUT));

    
    FragColor.rgb = mask_helper + pixel_haloed;



}

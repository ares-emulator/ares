#version 450

/* In this pass we calculate the average luminance of the scene.
 * It is cheaply obtained by the use of mipmaps. */

#include "config.inc"

//The mipmap wideness needed by the highlights on the bezel.
//the higher, the smoother.
#define mip_lod 5

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float lod;
layout(location = 2) out vec2 vFuzzy_main_pass_stage_1;
layout(location = 3) out float vReduced_sample_time_is_now;
layout(location = 4) out float vAvglum_needed;
layout(location = 5) out float vBezel_or_Bloom;
layout(location = 6) out float vResSwitch_res_changed;
layout(location = 7) out vec2 vTexCoord_unscaled;

#include "includes/functions.include.slang"


void main() {
    gl_Position = global.MVP * Position;
   
    vTexCoord = TexCoord;
    vTexCoord_unscaled = TexCoord;
    if (DO_GAME_GEOM_OVERRIDE > 0.5)
        vTexCoord = content_geom_override(vTexCoord, GAME_GEOM_ASPECT, get_in_aspect(), GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM);

	//Get fuzzy mul and pow factor
		vFuzzy_main_pass_stage_1 = apply_fuzzy_main_pass_stage_1();    

    vReduced_sample_time_is_now = mod(params.FrameCount,AMBI_AVGLUM_DIVIDER);
    
    vAvglum_needed = DO_AMBILIGHT + DO_BEZEL + (DO_DYNZOOM*10.0) + DO_BLOOM + (RESSWITCH_GLITCH_SIZE*10.0) ;
    vBezel_or_Bloom = DO_BEZEL + DO_BLOOM;
    
    //RESSWITCH shaking:
        vResSwitch_res_changed = 0.0;
        if (abs(params.OutputSize.y - params.avglum_passFeedbackSize.y) / params.OutputSize.y > RESSWITCH_GLITCH_TRESHOLD)
            if (params.FrameCount > 30) //needed to avoid shaking at start
                vResSwitch_res_changed = 1.0;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float lod;
layout(location = 2) in vec2 vFuzzy_main_pass_stage_1;
layout(location = 3) in float vReduced_sample_time_is_now;
layout(location = 4) in float vAvglum_needed;
layout(location = 5) in float vBezel_or_Bloom;
layout(location = 6) in float vResSwitch_res_changed;
layout(location = 7) in vec2 vTexCoord_unscaled;

layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D flick_and_noise_pass;
layout(set = 0, binding = 3) uniform sampler2D avglum_passFeedback;

/* The following will allow to sample at reduced interval
   The scene change detection logic will take advantage of it
   So that quick fades will still be treated as scene changes
*/
// #define avglum_divider 4  <-- in config.inc

#include "includes/functions.include.slang"


#define RGB_TO_GRAYSCALE = vec3(0.299, 0.587, 0.114)
float get_avg_lum_from_mip() {
    vec3 mip = textureLod(flick_and_noise_pass, vec2(0.5,0.5),20.0).rgb;
     mip = apply_fuzzy_main_pass_stage_2(mip, vFuzzy_main_pass_stage_1);
    //return (mip.r+mip.g+mip.b)/3.0;
    return rgb_to_gray(mip);
}

float get_avg_lum_from_mip_nocc() {
    vec3 mip = textureLod(flick_and_noise_pass, vec2(0.5,0.5),20.0).rgb;
    return rgb_to_gray(mip);
}

//0.1 seems like a good tradeoff that works well with bloom exposire and fullscreen zoom
//...maybe bloom exposure works best at 0.05; you may consider another zone of smooth sampling.
#define SMOOTHNESS_SPEED 0.1

float get_avglum() {
    /* avglum alpha has some zones:
     * 0.25 -> reduced sampling, used for to aid scene detection
     * 0.75 -> smoothed_sampling, used to adapt fullscreen zoom and adaptive black
     * 0.50 -> smoothed_sampling, no color corrections.
     * RESSWITCH_SAMPLING_POINT (0.9) -> resolution change detection
     * 
     * anything else: continuous sampling
     */
            
    bool reduced_sampling_co  = is_first_inside_rect(vTexCoord, vec4(0.24,0.24 , 0.26,0.26) );
    bool smoothed_sampling_co = is_first_inside_rect(vTexCoord, vec4(0.74,0.74 , 0.76,0.76) );
    bool smoothed_sampling_co_nocc = is_first_inside_rect(vTexCoord, vec4(0.49,0.49 , 0.51,0.51)) ;
    #ifndef D3D_WORKAROUND
        if (reduced_sampling_co) {
            //in the reduced sampling zone, only update at right time; else return the previous frame.
            if (vReduced_sample_time_is_now == 1.0) 
                return get_avg_lum_from_mip();
            
            float avglum_feedback = texture(avglum_passFeedback,vTexCoord).a;
            avglum_feedback = max(avglum_feedback, 0.0); // <- Sanitize input to avoid glitches when enabling the option runtime.
            return avglum_feedback;
        }

        if (smoothed_sampling_co) {
            return mix(texture(avglum_passFeedback,vTexCoord).a, get_avg_lum_from_mip(), SMOOTHNESS_SPEED);
        }
        
        if (smoothed_sampling_co_nocc) {
            return mix(texture(avglum_passFeedback,vTexCoord).a, get_avg_lum_from_mip_nocc(), SMOOTHNESS_SPEED);
        }
    #endif
    
    //Outside specific areas, update continuously; this is used by bezel highlights.
    return get_avg_lum_from_mip();

}


void main() {
    /*Grab a mipmap from the flick_and_noise_pass.
      Calculate the average luminance from the smallest mipmap and put into the alpha channel
      So that can be used:
      1 - continuously at coordinate 0.5,0.5
      2 - sampled less often for scene detection logic at coordinate 0.25,0.25
      Put another small mipmap into the rgb channel to be used by the reflection code.
    */
    
    if ( vAvglum_needed == 0.0) return;
    
    vec4 pixel_out;
    
    //We also reserve a sampling point in alpha channel to store if the resolution has changed.
    //The value stored will be used by the first pass as an argument to sin() to shake the image.
    if (is_first_inside_rect(vTexCoord_unscaled, vec4(RESSWITCH_SAMPLING_POINT-0.01 , RESSWITCH_SAMPLING_POINT+0.01) )) {
        if (vResSwitch_res_changed == 1.0 ) {
            pixel_out.a = 1;  
        } else {
            #ifndef D3D_WORKAROUND
                pixel_out.a = max(texture(avglum_passFeedback,vTexCoord_unscaled).a - RESSWITCH_SHAKE_SPEED, 0.0);
            #else
                 pixel_out.a = 0.0;
            #endif
        }
        
    } else {
        pixel_out.a = get_avglum();
    }
    

    
    //Create a small mipmap to be used to light the bezel corners
    //and by the bloom pass to identify bright areas.
    if (vBezel_or_Bloom > 0.0) 
        pixel_out.rgb = textureLod(flick_and_noise_pass, vTexCoord, mip_lod).rgb;


    FragColor = pixel_out;
    
}

#version 450

// This is one of several passes needed to cheaply emulate the bloom effect.

#include "config.inc"
#include "includes/functions.include.slang"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vFuzzy_main_pass_stage_1;
layout(location = 2) out vec3 vTemperature_rgb;
layout(location = 3) out vec3 vTemperature_rgb_gamma_in;
layout(location = 4) out float vIn_aspect;
//layout(location = 5) out vec2 vCo_avglum;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    
    vIn_aspect = get_in_aspect();
    
    //Get fuzzy mul and pow factor
        vFuzzy_main_pass_stage_1 = apply_fuzzy_main_pass_stage_1();

    //BLOOM temperature note (**)    
    /*  We would need to revert color temperature before the bloom 
    * and then re-apply the temperature or bloom would colorize whites with the color
    * temperature value!
    * in theory the temperature "revert" should be done before the BLOOM_GAMMA (gamma in) application,
    * so before bloom pass 1, but it seems we can do it in the final stage by dividing bloomed 
    * for pow(vTemperature_rgb, BLOOM_GAMMA), apply bloom gains and tweaks, and then 
    * multiply bloomed for vTemperature_rgb
    */
    
    if (TEMPERATURE == 6500 || DO_CCORRECTION == 0.0) {
        vTemperature_rgb = vec3(1.0);
        vTemperature_rgb_gamma_in =  vec3(1.0);
    } else {
        vTemperature_rgb = kelvin2rgb(TEMPERATURE);
        vTemperature_rgb_gamma_in = pow(vTemperature_rgb, vec3(BLOOM_GAMMA));
    }
    //debug
    //vTemperature_rgb = vec3(1.0);
    //vTemperature_rgb_gamma_in = vec3(1.0);
    
}

#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vFuzzy_main_pass_stage_1;
layout(location = 2) in vec3 vTemperature_rgb;
layout(location = 3) in vec3 vTemperature_rgb_gamma_in;
layout(location = 4) in float vIn_aspect;
//layout(location = 5) in vec2 vCo_avglum;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D Source;
//layout(set = 0, binding = 2) uniform sampler2D avglum_pass;
layout(set = 0, binding = 3) uniform sampler2D colortools_and_ntsc_pass;
layout(set = 0, binding = 4) uniform sampler2D bloom_pass_finalFeedback;


//#include "includes/functions.include.slang"
#include "includes/blooms.include.slang"

#define offset 0.5890486225480862 // TAU/8.0/4.0*3.0

//The curve shoulder to map scene luminance to bloom mix strength
#define BLOOM_PUPIL_EXPOSURE_CURVE_SHOULDER 0.33

//How big is the zone in which the luminance will be computed for puple temporal adaption (range 2..20)
#define BLOOM_PUPIL_LUM_ZONE_W 6

//The speed of the pupil adaption
#define BLOOM_PUPIL_ADAPTION_SPEED 0.2


float get_avg_lum_from_mip(sampler2D tex, vec2 co, float lod) {
    vec3 mip = textureLod(colortools_and_ntsc_pass, co, lod).rgb;
    mip = apply_fuzzy_main_pass_stage_2(mip, vFuzzy_main_pass_stage_1);
    //return (mip.r+mip.g+mip.b)/3.0;

    float luminance_first_pass = texture(bloom_pass_finalFeedback,vTexCoord).a;
    luminance_first_pass = max(luminance_first_pass, 0.0); // <- Sanitize input to avoid glitches when enabling the option runtime.
    //temporal smooth.
    //the defined emulated eye adaption speed needs to be divided 
    //by the eye adaption strength (BLOOM_EYE_ADPT_SRT)

    float eye_adaption_speed;
    float luminance_from_feedback = rgb_to_gray(mip);
    
    if (DELTA_RENDER == 0.0) {
        //The following makes the adaption speed more constant:
        eye_adaption_speed = BLOOM_PUPIL_ADAPTION_SPEED * ( luminance_first_pass / luminance_from_feedback ) ;

        //The following is needed so that the speed is the same even for higher strength
        eye_adaption_speed = (eye_adaption_speed / BLOOM_EYE_ADPT_SRT);
            
        //The following simulates pupil inertia
        eye_adaption_speed = clamp(eye_adaption_speed, 1.0/BLOOM_EYE_INERTIA, 1.0);
    } else {
        //Temporal adaption speed is no good with deltarenter.
        eye_adaption_speed = 1.0;
    }

    float luminance = mix(luminance_first_pass, luminance_from_feedback, eye_adaption_speed);
    return luminance + eps;
    
    
}



void main() {
    if (DO_BLOOM == 0.0) return;
    
    vec3 bloomed = bloom(
        Source,
        vTexCoord,
        params.SourceSize,
        vec2(BLOOM_SIZE),
        BLOOM_QUALITY,
        offset,
        0.0
    );
    
    bloomed = bloomed / vTemperature_rgb_gamma_in;  //see note (**) in vertex shader
    
    
    bloomed = pow(bloomed,vec3(BLOOM_GAMMA_OUT));
    bloomed = pixel_push_luminance(bloomed, BLOOM_POWER);
    
    float fbloom_mix = BLOOM_MIX;
    float bloom_mix_adapted = 1.0;
    float over_white_correction = 1.0;
    
    
    if (BLOOM_OVER_WHITE < 1.0) {
        

    //Apply other content coords modifiers        
        //Use mipmap available in avglum_pass to identify bright areas.
        //vec3 avglum = texture(avglum_pass,vCo_avglum).rgb;
        
        #define ZONE_LUMINANCE_LOD 2.0 //Higher values lead to ugly glitch when panning or banding in static content.
        
        vec3 avglum = textureLod(colortools_and_ntsc_pass, vTexCoord, ZONE_LUMINANCE_LOD).rgb;
        float b_brightness = max(avglum.b,max(avglum.r,avglum.g));
        b_brightness *= 1-BLOOM_OVER_WHITE;
        over_white_correction =1- b_brightness;
    }
    
    bloomed*=BLOOM_POWER;
    
    //                                                  (Branching with (if BLOOM_EYE_ADPT_SRT == 0.0) does not gain anything.)
    // Modulate exposure over time too.
    //Smooth luminance with specified lod in this alpha channel
    float zone_luminance = get_avg_lum_from_mip(colortools_and_ntsc_pass, vTexCoord, BLOOM_PUPIL_LUM_ZONE_W);
    float smoothed_avglum = zone_luminance * BLOOM_EYE_ADPT_SRT;
    bloom_mix_adapted = max(0.0, BLOOM_MIX - smoothed_avglum);
    
    
    if (BLOOM_BYPASS == 2.0) fbloom_mix = 1.0;
    
    
    float pow_exp = BLOOM_PUPIL_EXPOSURE_CURVE_SHOULDER - smoothed_avglum * BLOOM_PUPIL_EXPOSURE_CURVE_SHOULDER;
    pow_exp = max(0.0, pow_exp); //avoid negative exps.
    bloom_mix_adapted = fbloom_mix * ( 1- pow(smoothed_avglum, pow_exp) );
    
    
    if (BLOOM_BYPASS != 1.0)
        bloomed = bloomed * bloom_mix_adapted * over_white_correction ;
    

    bloomed = bloomed * vTemperature_rgb; //see note (**) in vertex shader
    
    FragColor = vec4(bloomed, zone_luminance);   
    
    //FragColor = vec4(zone_luminance); //Use me to see luminosity zones
    
}

#version 450

/* This pass simulates the presence of a led strip placed on the back of the virtual screen */

#include "config.inc"
#include "includes/functions.include.slang"

#define PastSampler ambi_temporal_passFeedback

#define smp_internalness AMBI_SMP_INT_OFFSET  //The distance between the screen border and the color sampling point
#define leds_per_strip 8   //How many leds per border
#define radius 0.001       //The base radius of the emitted light (tuned by user parameter later)

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float vLedBorder_min;
layout(location = 2) out float vLedBorder_max;
layout(location = 3) out float vLed_step;
layout(location = 4) out float lod;
layout(location = 5) out vec2 pre_pass_coords;
layout(location = 6) out float vIs_rotated;
layout(location = 7) out float vAMBI_STRETCH;
layout(location = 8) out float vAMBI_POWER;
layout(location = 9) out float vSmpBorder_min;
layout(location = 10) out float vSmpBorder_max;
layout(location = 11) out float vSmp_step;
layout(location = 12) out float vMod_FrameCount_FrameDivider;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;

    //Min/Max points to sample colors:
        vSmpBorder_min = smp_internalness;
        vSmpBorder_max = 1.0-smp_internalness;

        if (DO_GAME_GEOM_OVERRIDE > 0.5) {
            vSmpBorder_min = content_geom_override_zoom(vec2(vSmpBorder_min), GAME_GEOM_ASPECT, get_in_aspect(), GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM).x;
            vSmpBorder_max = content_geom_override_zoom(vec2(vSmpBorder_max), GAME_GEOM_ASPECT, get_in_aspect(), GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM).x;
        }

    bool isrotated = is_rotated();
    vIs_rotated = float(isrotated);

    if (isrotated)
        vAMBI_STRETCH = AMBI_STRETCH_VERTICAL;
            else
        vAMBI_STRETCH = AMBI_STRETCH;

        //Keep leds in place when content geometry zoom changes
        float ambi_int_offset = AMBI_INT_OFFSET;
        if (DO_GAME_GEOM_OVERRIDE + DO_BEZEL > 1.5) {
                ambi_int_offset = content_geom_override_zoom    (vec2(ambi_int_offset), GAME_GEOM_ASPECT, get_in_aspect(), GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM).x;
        }
        
        //Move virtual leds position inside to compensate for widen effect
        float led_internalness = ambi_int_offset + (vAMBI_STRETCH *0.025);

       
    //less internalness when using bezel:
        led_internalness -= 0.07 * DO_BEZEL;
       
        //led_internalness = -0.18   ;

    //Min/Max points to show Leds;
        vLedBorder_min=led_internalness;
        vLedBorder_max=1.0-led_internalness;

    vLed_step = (vLedBorder_max - vLedBorder_min)  / (leds_per_strip-1);
    vSmp_step = (vSmpBorder_max - vSmpBorder_min)  / (leds_per_strip-1);

    //Calc a lod for a texture sized led_strip x led_strip
        lod = log2(global.flick_and_noise_passSize.y / leds_per_strip) - 1;


   //Scale to the original aspect
    float in_aspect = get_in_aspect();

    float Do_Tate = need_tate(isrotated);

    
    if ( need_NO_integer_scale() )
        pre_pass_coords = get_scaled_coords_aspect(TexCoord,global.FinalViewportSize, in_aspect, isrotated) + vec2(0.00001);
            else
        pre_pass_coords = integer_scale(TexCoord, in_aspect, isrotated, Do_Tate, GAME_GEOM_INT_SCALE-1.0 ) ;


    if (DO_GLOBAL_SHZO >0.5)
        pre_pass_coords = zoom(pre_pass_coords + vec2(-GLOBAL_OFFX, -GLOBAL_OFFY), GLOBAL_ZOOM );

    if (DO_GAME_GEOM_OVERRIDE > 0.5) {
        pre_pass_coords = content_geom_override_zoom(pre_pass_coords, GAME_GEOM_ASPECT, get_in_aspect(), GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM);
    }

    if (DO_BEZEL > 0.5)
        pre_pass_coords = zoomout_coords(pre_pass_coords, -get_BEZEL_INNER_ZOOM());


    //Every cycle counts!
        vAMBI_POWER = AMBI_POWER -1;

        
    //3.0 makes scene detection not working.
        #define FRAME_DIVIDER 2.0
        vMod_FrameCount_FrameDivider = mod(params.FrameCount, FRAME_DIVIDER);
        
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float vLedBorder_min;
layout(location = 2) in float vLedBorder_max;
layout(location = 3) in float vLed_step;
layout(location = 4) in float lod;
layout(location = 5) in vec2 pre_pass_coords;
layout(location = 6) in float vIs_rotated;
layout(location = 7) in float vAMBI_STRETCH;
layout(location = 8) in float vAMBI_POWER;
layout(location = 9) in float vSmpBorder_min;
layout(location = 10) in float vSmpBorder_max;
layout(location = 11) in float vSmp_step;
layout(location = 12) in float vMod_FrameCount_FrameDivider;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D PastSampler;
layout(set = 0, binding = 2) uniform sampler2D avglum_pass;
layout(set = 0, binding = 3) uniform sampler2D avglum_passFeedback;
layout(set = 0, binding = 4) uniform sampler2D colortools_and_ntsc_pass;



vec3 ambi_push_pass(vec2 coords, float f_lod) {
    vec2 smp_coords = coords;
    if (AUTOCROP_MAX > 0.0) {
        float autocrop_zoom = texture(avglum_pass, AUTOCROP_SAMPLING_POINT).a;
        smp_coords = zoom(smp_coords, autocrop_zoom);
    }
    
    vec3 pixel_out = textureLod(colortools_and_ntsc_pass, smp_coords, f_lod).rgb;
    pixel_out = pow(pixel_out, vec3(AMBI_GAMMA)); 

    pixel_out = pixel_push_luminance(pixel_out, vAMBI_POWER);
    return pixel_out;
}


float fcircle_smooth(vec2 coords, vec2 middle, float f_radius, float FALLOFF) {
    float fdistance = distance(middle, vec2(coords.x, coords.y));
    return (1-smoothstep(f_radius-FALLOFF, f_radius+FALLOFF, fdistance));
}

#define tol_start 0.08 //skip tolerance
#define tol_end   1-tol_start



vec3 ambi_pre_pass(vec2 coords) {
    if (DO_TILT == 1.0)
        coords = tilt(coords, vIs_rotated, TILT_X);

    //Skip coords in the rect "under the tube"
   // if (is_first_inside_rect(coords, vec4(tol_start, tol_start, tol_end ,tol_end)))
   //     return vec3(0.0);
    vec2 skip_coords = coords;
    
    if (DO_GAME_GEOM_OVERRIDE > 0.5 )
        skip_coords = content_geom_override_zoom_inv(skip_coords, GAME_GEOM_ASPECT, get_in_aspect(), GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM);
    
    if (is_first_inside_rect(skip_coords, vec4(tol_start, tol_start, tol_end ,tol_end)))
        return vec3(0.0);

    //Stretch the lights?
        float z1=distance(vec2(0.5,0.5), coords);
        vec2 zoomed_coords = zoomxy(coords, vec2(1+z1));
        coords = mix(coords, zoomed_coords, vAMBI_STRETCH*0.5);
        float lAMBI_FALLOFF = AMBI_FALLOFF + (z1 * vAMBI_STRETCH*0.5);

    //Finally, emulate leds.
    vec3 pixel_out = vec3(0.0);

    vec2 smp_point; //The coords where the source color is sampled
    vec2 led_point; //The coords where the led is shown



    //LEFT strip
    led_point = vec2(vLedBorder_min);
    smp_point = vec2(vSmpBorder_min);
    for (int i=1 ; i <= leds_per_strip ; i++) {
        float circle_shape = fcircle_smooth(coords, led_point, radius, lAMBI_FALLOFF);
        vec3  circle_color = ambi_push_pass(smp_point, lod);
        pixel_out += circle_shape * circle_color;
        led_point.y += vLed_step;
        smp_point.y += vSmp_step;
    }

    //RIGHT strip
    led_point = vec2(vLedBorder_max, vLedBorder_min);
    smp_point = vec2(vSmpBorder_max, vSmpBorder_min);
    for (int i=1 ; i <= leds_per_strip ; i++) {
        float circle_shape = fcircle_smooth(coords, led_point, radius, lAMBI_FALLOFF);
        vec3  circle_color = ambi_push_pass(smp_point, lod);
        pixel_out += circle_shape * circle_color;
        led_point.y += vLed_step;
        smp_point.y += vSmp_step;
    }

    //TOP strip
    led_point = vec2(vLedBorder_min + vLed_step, vLedBorder_min);
    smp_point = vec2(vSmpBorder_min + vSmp_step, vSmpBorder_min);
    for (int i=1 ; i <= leds_per_strip-2 ; i++) {
        float circle_shape = fcircle_smooth(coords, led_point, radius, lAMBI_FALLOFF);
        vec3  circle_color = ambi_push_pass(smp_point, lod);
        pixel_out += circle_shape * circle_color;
        led_point.x += vLed_step;
        smp_point.x += vSmp_step;
    }

    //BOTTOM strip
    led_point = vec2(vLedBorder_min + vLed_step, vLedBorder_max);
    smp_point = vec2(vSmpBorder_min + vSmp_step, vSmpBorder_max);
    for (int i=1 ; i <= leds_per_strip-2 ; i++) {
        float circle_shape = fcircle_smooth(coords, led_point, radius, lAMBI_FALLOFF);
        vec3  circle_color = ambi_push_pass(smp_point, lod);
        pixel_out += circle_shape * circle_color;
        led_point.x += vLed_step;
        smp_point.x += vSmp_step;
    }
    //pixel_out = pow(pixel_out, 1/vec3(AMBI_GAMMA));
    return pow(pixel_out, 1/vec3(1+vAMBI_STRETCH));
}



#define MAX_STEPS AMBI_STEPS

float ambi_step(float start, float end, float mystep) {
    float diff = start-end;
    if (abs(diff) < mystep) return end;
    //Does not worth to try to unbranch:
    if (start >= end)
        return start - mystep;
            else
        return start + mystep;

}

vec3 ambi_step_rgb(vec3 s,vec3 d, vec3 mystep){
    //step fade from s to d
    return vec3 ( ambi_step(s.r, d.r, mystep.r),
                  ambi_step(s.g, d.g, mystep.g),
                  ambi_step(s.b, d.b, mystep.b)
                );
}


vec4 pixel_ambilight(vec2 local_pre_pass_coords) {

    vec4 past_pixel_vec4 = texture(PastSampler, vTexCoord);
    vec3 present_pixel =  ambi_pre_pass(local_pre_pass_coords).rgb;
    vec3 past_pixel =     past_pixel_vec4.rgb;

    float scene_change_remaining = past_pixel_vec4.a;
    
    #ifndef D3D_WORKAROUND
        float past_avg_lum    = texture(avglum_passFeedback,vec2(0.25,0.25)).a;
        float present_avg_lum = texture(avglum_pass        ,vec2(0.25,0.25)).a;
        float diff_avg_lum = abs(past_avg_lum - present_avg_lum);

        if (diff_avg_lum >= AMBI_SCENE_CHG_THRSHLD) {
            scene_change_remaining = 1/AMBI_FAST_STEP;
        }
    #endif 
    
    // Are we changing scene?
    vec3 mystep;
    if (scene_change_remaining > 0.0) {
        mystep = vec3(max(1.0/MAX_STEPS, AMBI_FAST_STEP)); // <- Never slow down fades due to fast step when changing scene
        scene_change_remaining -=  AMBI_FAST_STEP;
    } else {
        mystep = abs((past_pixel - present_pixel) / MAX_STEPS); //OK
    }
    
    return vec4(ambi_step_rgb(past_pixel, present_pixel, mystep), scene_change_remaining);
}


void main() {
    if (DO_AMBILIGHT != 1.0)
        return;
    
    //Nvidia + flycast core + glcore goes crazy with modulo operation and instead of returning 0,1,2 0,1,2 they returns 1,2,3 1,2,3 and so on.
    //Don't check for != 0.0 here, but use != 1.0.
    //Also this particular bug does not seem to happen in the last pass.
    
    if ( vMod_FrameCount_FrameDivider != 1.0 ) { 
        vec4 past_sampler = texture(PastSampler, vTexCoord);
        past_sampler = max(past_sampler, 0.0); // <- Sanitize input to avoid glitches when enabling the option runtime.
        FragColor = past_sampler;
        return;
    }


    FragColor = pixel_ambilight(pre_pass_coords);
}

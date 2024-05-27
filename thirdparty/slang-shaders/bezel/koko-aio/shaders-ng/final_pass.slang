#version 450


/* This pass:
 * Composes the previous passes
 * Does masks, spot, bezel, vignette, background image (anything else?)
 */

                                #define DEBUG_TEXTURE Original
                                //#define DEBUG_TEXTURE colortools_and_ntsc_pass
//#define DEBUG_PRINT_VALUE
//#define DEBUG_QUAD_SPLIT
//#define DEBUG_DUAL_SPLIT_X
//#define DEBUG_DUAL_SPLIT_Y
//#define DEBUG_DUAL_SPLIT_Y_GAMMA
//#define DEBUG_DUAL_SPLIT_MIRROR_X
//#define DEBUG_DUAL_SPLIT_MIRROR_X_GAMMA
//#define DEBUG_SHOW_CLIP


#include "config.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vOutputCoord;
layout(location = 2) out vec2 vBg_img_coords;
layout(location = 3) out float vIsRotated;
layout(location = 4) out float vScanlinePeriod;
layout(location = 5) out float vInterlace_offset;
layout(location = 6) out float vIn_aspect;
layout(location = 7) out float vBEZEL_INNER_ZOOM_adapted;
layout(location = 8) out float vDo_Tate;
layout(location = 9) out vec3 vDotMat_Grid_Color;
layout(location = 10) out vec4 vPG_offsets_and_size;
layout(location = 11) out vec2 vPG_freq_base_screen;
layout(location = 12) out vec2 vPG_freq_base_screen_unfloored;
layout(location = 13) out float vPG_period_multiplier_x;
layout(location = 14) out vec2 vPG_OriginalSize_tated_mul_pi;
layout(location = 15) out vec2 vPG_OutputSize_tated;
layout(location = 16) out vec2 vPG_OutputCoord_tated;
layout(location = 17) out float vDotMat_Grid_Sharpness;
layout(location = 18) out float vPG_mask_height;
layout(location = 19) out float vDeltaRenderOk;
layout(location = 20) out float vDo_pixelgrid_h;
layout(location = 21) out float vFake_scanline_period;
layout(location = 22) out float vPG_y_shift;

#ifdef DEBUG_PRINT_VALUE
    layout(location = 31) out float vfprintvalue; 
#endif

// I_am_a_placeholder_keep_reading;
//..to remind you that location = 31 is the last accepted by vulkan
//or Retroarch crashes or nvidia starts to give Xids.
//unsure if the issue is with retroarch or with vulkan specs.
//If in short, consider to pack more datas into vec2,3,4.
//But bear in mind that exceding the limit leads to
//slowdowns.

#include "includes/functions.include.slang"

#define PI_15 4.71238898038469 //pi*1.5


vec2 get_bg_image_coords(vec2 in_vOutputCoord, bool isrotated) {
    vec2 bg_img_coords = in_vOutputCoord;

    //   0 = auto
    //   1, -1 = identity
    // >+1 = manual rotation for straight games
    // <-1 = manual rotation for rotated games

    //Auto rotation, no user change allowed:
    if (BG_IMAGE_ROTATION == 0.0 && isrotated ) {
        //handle automatic rotation of bg image for rotated games
        bg_img_coords.xy = bg_img_coords.yx;
        bg_img_coords.y = 1 - bg_img_coords.y;
        return bg_img_coords; // <-- for legibility
    }


    //User say image is rotated (selected a negative value),
    //flip coords but allow user customization.
    //This is needed for correct aspect handling on rotated games.
    if (BG_IMAGE_ROTATION < 0.0) {
        bg_img_coords.xy = bg_img_coords.yx;
        bg_img_coords.y = 1 - bg_img_coords.y;
    }

    if (abs(BG_IMAGE_ROTATION) == 1.0) // identity
        bg_img_coords.xy = bg_img_coords.xy;

    else if (abs(BG_IMAGE_ROTATION) == 2.0) // +180
       bg_img_coords.xy =   vec2(1-bg_img_coords.x, 1 - bg_img_coords.y);

    else if (abs(BG_IMAGE_ROTATION) == 3.0) // 0 mirror x
       bg_img_coords.xy =   vec2( 1 - bg_img_coords.x, bg_img_coords.y);

    else if (abs(BG_IMAGE_ROTATION) == 4.0) // 0 mirror y
       bg_img_coords.xy =   vec2( bg_img_coords.x, 1 - bg_img_coords.y);

    else if (abs(BG_IMAGE_ROTATION) == 5.0) // +90
        bg_img_coords.xy = vec2(bg_img_coords.y, 1- bg_img_coords.x);

    else if (abs(BG_IMAGE_ROTATION) == 6.0) // +270
       bg_img_coords.xy =   vec2( 1- bg_img_coords.y, bg_img_coords.x);

    else if (abs(BG_IMAGE_ROTATION) == 7.0) // 90 mirror x
       bg_img_coords.xy =   vec2(bg_img_coords.y, bg_img_coords.x);

    else if (abs(BG_IMAGE_ROTATION) == 8.0) // +90 mirror y
        bg_img_coords.xy = vec2(1-bg_img_coords.y, 1- bg_img_coords.x);

    bg_img_coords += vec2( BG_IMAGE_OFFX,BG_IMAGE_OFFY);

    bg_img_coords=zoom(bg_img_coords, BG_IMAGE_ZOOM);

    return bg_img_coords;
}

vec2 get_zooms_modifier(float do_tate, bool bNeed_NO_integer_scale, bool isrotated) {
    // This function is meant to live in vertex shader; its purpose is to
    // give the final scale factor by taking in consideration various zoom modifiers.
    //It works by calculating the new 0.0 and 1.0 coords then measuring the new distance between them

    // WARNING: This has to be in sync with the code, so everytime a scaling flow is modified, this needs to be updated. WARNING
    // WARNING: This has to be in sync with the code, so everytime a scaling flow is modified, this needs to be updated. WARNING
    // WARNING: This has to be in sync with the code, so everytime a scaling flow is modified, this needs to be updated. WARNING

    vec2 co_scaled_min = vec2(0.0);
    vec2 co_scaled_max = vec2(1.0);

    if ( bNeed_NO_integer_scale ) {
        co_scaled_min = get_scaled_coords_aspect(co_scaled_min, global.FinalViewportSize, vIn_aspect, isrotated);
        co_scaled_max = get_scaled_coords_aspect(co_scaled_max, global.FinalViewportSize, vIn_aspect, isrotated);
    } else {
        co_scaled_min = integer_scale(co_scaled_min, vIn_aspect, isrotated, do_tate, GAME_GEOM_INT_SCALE-1.0 );
        co_scaled_max = integer_scale(co_scaled_max, vIn_aspect, isrotated, do_tate, GAME_GEOM_INT_SCALE-1.0 );
    }


    co_scaled_min = (zoom(co_scaled_min + vec2(-GLOBAL_OFFX, -GLOBAL_OFFY), GLOBAL_ZOOM ) * DO_GLOBAL_SHZO) +
                (co_scaled_min * (1-DO_GLOBAL_SHZO) );
    co_scaled_max = (zoom(co_scaled_max + vec2(-GLOBAL_OFFX, -GLOBAL_OFFY), GLOBAL_ZOOM ) * DO_GLOBAL_SHZO) +
                (co_scaled_max * (1-DO_GLOBAL_SHZO) );

    if (DO_BEZEL == 1.0) {
        co_scaled_min = zoomout_coords(co_scaled_min, -vBEZEL_INNER_ZOOM_adapted);
        co_scaled_max = zoomout_coords(co_scaled_max, -vBEZEL_INNER_ZOOM_adapted);
    }
 	if (DO_GAME_GEOM_OVERRIDE == 1.0) {
		co_scaled_min = content_geom_override_zoom(co_scaled_min, GAME_GEOM_ASPECT, vIn_aspect, GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM);
        co_scaled_max = content_geom_override_zoom(co_scaled_max, GAME_GEOM_ASPECT, vIn_aspect, GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM);
    }

    //vfprintvalue = 1/(co_scaled_max.y - co_scaled_min.y);
    return 1 / vec2(co_scaled_max.x - co_scaled_min.x,
                co_scaled_max.y - co_scaled_min.y );

}


void main() {
    gl_Position = global.MVP * Position;
    bool bIsRotated = is_rotated();
    vIsRotated = float(bIsRotated);

    vDo_Tate = need_tate(bIsRotated);

    vIn_aspect = get_in_aspect();

    //Calculate vTexcoord as fractional or integer scaled?
        bool bNeed_NO_integer_scale = need_NO_integer_scale();
        if ( bNeed_NO_integer_scale )
            vTexCoord = get_scaled_coords_aspect(TexCoord,global.FinalViewportSize, vIn_aspect, bIsRotated);
        else
            vTexCoord = integer_scale(TexCoord, vIn_aspect, bIsRotated, vDo_Tate, GAME_GEOM_INT_SCALE-1.0 ) + vec2( GAME_GEOM_OFF_FIX /10000);

    vTexCoord += 0.00001;
    

    

    //if (DO_GLOBAL_SHZO >0.5)
    //    vTexCoord = zoom(vTexCoord + vec2(-GLOBAL_OFFX, -GLOBAL_OFFY), GLOBAL_ZOOM );
    //..unbranched previous
        vTexCoord = (zoom(vTexCoord + vec2(-GLOBAL_OFFX, -GLOBAL_OFFY), GLOBAL_ZOOM ) * DO_GLOBAL_SHZO) +
                    (vTexCoord * (1-DO_GLOBAL_SHZO) );

    vOutputCoord = TexCoord ;

    vBg_img_coords =  get_bg_image_coords(vOutputCoord, bIsRotated);

    vBEZEL_INNER_ZOOM_adapted = get_BEZEL_INNER_ZOOM() * DO_BEZEL;

    //Help scanline/pixelgrid code too:
        bool bIs_Interlaced = is_interlaced();

        //Scanline period:
            vScanlinePeriod = 1.0; //Base 1x period
            vInterlace_offset = 0.0; //No interlace

            if (bIs_Interlaced && PIXELGRID_INTR_DISABLE_Y != 2.0) {
                //Halve the scanline period, since they will interlace:
                vScanlinePeriod = 0.5;
                //Alternate scanlines on even/odd frames
                if (params.FrameCount % 2 == 0.0)
                    vInterlace_offset = PI_15;
            }


    //Calc dotmat grid color
        vDotMat_Grid_Color = vec3(DOT_M_G_BRT);
        if (DO_DOT_MATRIX + DO_CCORRECTION > 1.1) {
            vDotMat_Grid_Color = color_tools(vec3(DOT_M_G_BRT), kelvin2rgb(TEMPERATURE));
            //Since we modify grid brightness via a specific use parameter, explicitely multiply it by that:
                vDotMat_Grid_Color *= DOT_M_G_BRT ;
        }


    //Pixelgrid: get mask type; .rgb contains layout, .a contains the size.
    //we use 2 different layouts for floored and unfloored coords, because
    //with unfloored coords we move the gap half beginning, half end,
    //while with floored coords it must have its own integer spot.
        if (PIXELGRID_SIZE_W == 0.0 || PIXELGRID_MUL_X != 1.0)
            vPG_offsets_and_size = PG_get_hmask_preset_unfloored() ;
                else
            vPG_offsets_and_size = PG_get_hmask_preset_floored() ;
       //scale offsets to be used by sin:
        #define PG_H_COUNT vPG_offsets_and_size.a
        vPG_offsets_and_size.rgb *= (pi / PG_H_COUNT);

    //Pixelgrid: Calc base freqs
        //Tate ?
            if (vDo_Tate == 0.0) { 
                vPG_OutputSize_tated   = params.OutputSize.xy;
                vPG_OutputCoord_tated  = vOutputCoord.xy + vec2(0.00001);
                vPG_OriginalSize_tated_mul_pi = params.OriginalSize.xy * pi;
            } else {
                vPG_OutputSize_tated   = params.OutputSize.yx;
                vPG_OutputCoord_tated  = vOutputCoord.yx;
                vPG_OriginalSize_tated_mul_pi = params.OriginalSize.yx * pi;
            }


        // screen coords needs flooring, but unfortunately floor() does not work well in vertex shader, so calc as much as possible without floor()
            vPG_period_multiplier_x = 1/PIXELGRID_MUL_X;
            vPG_freq_base_screen = pi * vec2(1/PG_H_COUNT, 0.5);

        //the following unfloored coords are needed by sin/cos later to understand if we are on an even or odd mask
            vPG_freq_base_screen_unfloored = pi * vec2(1/PG_H_COUNT, 0.5) * (vPG_OutputCoord_tated * vPG_OutputSize_tated);

    //pixelgrid: doublescan?
                if (need_doublescan() )
                    vPG_OriginalSize_tated_mul_pi *= 2.0;

    //Pixelgrid: adapt period multiplier
        if (vPG_period_multiplier_x < 0.0) vPG_period_multiplier_x = 1/-vPG_period_multiplier_x;



    //Pixelgrid Y mask: adapt mask height
        vPG_mask_height = PIXELGRID_Y_MASK_HEIGHT;
        if (vPG_mask_height < 0.0) vPG_mask_height = 1/-vPG_mask_height;

    //Pixelgrid Y mask: Select an y shift that gives the sharpest lines
        bool y_mask_align_to_screen_lines = (PIXELGRID_Y_MASK_SHIFT < 0.0)      && //Engage when param is < 0.0 (auto) and
                                            (PIXELGRID_Y_MASK_COORDS == 1.0)    && //only when when using screen coords and
                                            (fract(1.0/vPG_mask_height) == 0.0);   //only when the multiplier is integer
    
        if (y_mask_align_to_screen_lines ) {
            vPG_y_shift = vPG_mask_height * (pi / 4.0);
        } else {
            vPG_y_shift = PIXELGRID_Y_MASK_SHIFT * pi ;
        }
        
        
    //Pixelgrid: do horizontal if enabled, but skip it if user doesn't want it on interlaced content.
        vDo_pixelgrid_h = float( pixelgrid_h_needed() );


    //Dot matrix: measure the final picture size to choose a right sharpness level
    //For simplicity and (my) mental health, take only y into account.
        vDotMat_Grid_Sharpness = DOT_M_G_SHARP;
        if (DOT_M_G_SHARP == 0.0) {
            float zooms_modifier  = get_zooms_modifier(vDo_Tate, bNeed_NO_integer_scale, bIsRotated).y;
            float dotmat_insize  = global.flick_and_noise_passSize.y;
            float dotmat_outsize = global.FinalViewportSize.y  * zooms_modifier;
            #ifdef DEBUG_PRINT_VALUE
                vfprintvalue = dotmat_outsize / dotmat_insize;
            #endif
            vDotMat_Grid_Sharpness = (dotmat_outsize / dotmat_insize);
            vDotMat_Grid_Sharpness = vDotMat_Grid_Sharpness * 3.6 - 3.1;
            vDotMat_Grid_Sharpness = clamp(vDotMat_Grid_Sharpness, 0.1, 20.0);
            //vfprintvalue = vDotMat_Grid_Sharpness;
        }
        
    vDeltaRenderOk = is_deltarender_allowed(bIs_Interlaced); //carries DR mandatory conditions, including forced refreshed frames
        
    // Fake or Faker integer scanlines:
    // We are going to compute an amount of lines that is integer multiple of the screen lines and that
    // is near to the real input/core lines
    vFake_scanline_period = 0.0;
    
    
    // For automatic Fake scanline managment, we compute a treshold under which
    // fake integer scanlines will be used.
    // (Logic is skipped if integer scaling is requested.)
        bool very_lowres_y = false;
        #define MIN_SCREEN_CORE_RATIO 3.5 //We want at least that screen lines per single scanline.
        if ( bNeed_NO_integer_scale ) {
            vec4 out_size = params.OutputSize;
            vec4 ori_size = params.OriginalSize;
            vec2 zoom_modifier = get_zooms_modifier(vDo_Tate, bNeed_NO_integer_scale,bIsRotated);
            if (vDo_Tate == 1.0) {
                out_size.xy = out_size.yx;
                ori_size.xyzw = ori_size.yxwz;
                zoom_modifier.xy = zoom_modifier.yx;
            }
            float OutputHeightZoomed = out_size.y*zoom_modifier.y;
            float screen_core_ratio = OutputHeightZoomed / ori_size.y;
            if (screen_core_ratio < MIN_SCREEN_CORE_RATIO)
                very_lowres_y = true;
        }
    
    //Conditions for fake scanlines are: 
    // - user wants fake/faker integer scanlines when content is interlaced (*1)
    //   or
    // - user always wants fake integer scanlines. (*2)
    //   or
    // - user wants automatic fake integer scanlines managment and screen is too low res (*3)
    //   or
    // - user wants fake integer scanlines when content is double-scanned (*4) // <-- edit, ditched as it seems useless to me.
    

    
    bool fake_scanlines_needed =  (PIXELGRID_INTR_DISABLE_Y <= -1.0 && bIs_Interlaced) || //(*1)
                                  (PIXELGRID_COREY_FAKE_SCAN == 1.0) ||                   //(*2)
                                  (PIXELGRID_COREY_FAKE_SCAN == -1.0 && very_lowres_y);    //(*3)
                                  //(PIXELGRID_COREY_FAKE_SCAN == 2.0 &&  need_doublescan() ); //(*4)
                
                                  
    if ( fake_scanlines_needed ) {
        vec4 out_size = params.OutputSize;
        vec4 ori_size = params.OriginalSize;
        vec2 out_coord = vOutputCoord;
        vec2 zoom_modifier = get_zooms_modifier(vDo_Tate, bNeed_NO_integer_scale,bIsRotated);
        if (vDo_Tate == 1.0) {
            out_size.xy = out_size.yx;
            ori_size.xyzw = ori_size.yxwz;
            out_coord.xy = out_coord.yx;
            zoom_modifier.xy = zoom_modifier.yx;
        }

        // Modify the y line size ?
            float target_line_size = ori_size.w ;
            if ( need_doublescan() ) {
                target_line_size /= 2.0; //halve line size for doublescan
            } else if (PIXELGRID_INTR_DISABLE_Y == -2.0 && bIs_Interlaced) {
                target_line_size /= 1.5; //reduce line size for fakeR scanlines on interlaced screens.
            }
            
        // compute a rounded number of lines that integer fits the screen
            float rounded_denominator = round( (out_size.y*zoom_modifier.y) * target_line_size);
                  rounded_denominator = max(rounded_denominator, 2.0); // avoid division by 0  and ensure (, 2.0)
                                                                       // scanline gaps are drawn even on tiny output sizes.
            
            float fake_scanlines_numlines = out_size.y / rounded_denominator;
            
        //Compute the final fake scanline period
            vFake_scanline_period = pi * fake_scanlines_numlines * out_coord.y;
                
        //This somehow aligns better the scanline to the screen line:
            vInterlace_offset+=1.0;
    }
    

}



#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vOutputCoord;
layout(location = 2) in vec2 vBg_img_coords;
layout(location = 3) in float vIsRotated;
layout(location = 4) in float vScanlinePeriod;
layout(location = 5) in float vInterlace_offset;
layout(location = 6) in float vIn_aspect;

layout(location = 7) in float vBEZEL_INNER_ZOOM_adapted;
layout(location = 8) in float vDo_Tate;
layout(location = 9) in vec3 vDotMat_Grid_Color;
layout(location = 10) in vec4 vPG_offsets_and_size;
layout(location = 11) in vec2 vPG_freq_base_screen;
layout(location = 12) in vec2 vPG_freq_base_screen_unfloored;
layout(location = 13) in float vPG_period_multiplier_x;
layout(location = 14) in vec2 vPG_OriginalSize_tated_mul_pi;
layout(location = 15) in vec2 vPG_OutputSize_tated;
layout(location = 16) in vec2 vPG_OutputCoord_tated;
layout(location = 17) in float vDotMat_Grid_Sharpness;
layout(location = 18) in float vPG_mask_height;
layout(location = 19) in float vDeltaRenderOk;
layout(location = 20) in float vDo_pixelgrid_h;
layout(location = 21) in float vFake_scanline_period;
layout(location = 22) in float vPG_y_shift;

#ifdef DEBUG_PRINT_VALUE
    layout(location = 31) in float vfprintvalue;
#endif

layout(location = 0) out vec4 FragColor;


layout(set = 0, binding = 1) uniform sampler2D backdrop;
layout(set = 0, binding = 2)  uniform sampler2D bloom_pass_final;
layout(set = 0, binding = 3)  uniform sampler2D reflected_blurred_pass;
layout(set = 0, binding = 4)  uniform sampler2D ambi_temporal_pass;
layout(set = 0, binding = 5)  uniform sampler2D avglum_pass;
layout(set = 0, binding = 6)  uniform sampler2D monitor_body_straight;
layout(set = 0, binding = 7)  uniform sampler2D monitor_body_curved;
layout(set = 0, binding = 8)  uniform sampler2D bg_under;
layout(set = 0, binding = 9)  uniform sampler2D bg_over;
//layout(set = 0, binding = 10) uniform sampler2D in_glow_pass_x;
layout(set = 0, binding = 11) uniform sampler2D in_glow_pass;
layout(set = 0, binding = 12) uniform sampler2D halo_pass;
#ifndef D3D_WORKAROUND
    layout(set = 0, binding = 13) uniform sampler2D final_passFeedback;
#endif
layout(set = 0, binding = 15) uniform sampler2D DEBUG_TEXTURE;



#define RECT01 vec4(0.0, 0.0, 1.0, 1.0)
#define HALF_PI 1.5707963267949
#define QUARTER_PI 0.785398163397448

#include "includes/functions.include.slang"


vec2 vOutputCoord_adapted;
vec2 get_vOutputCoord_adapted() {
    return vOutputCoord_adapted;
}


vec3 fn_pixel_nightify(vec3 color_in, float strength, vec3 ambilight) {
    if (strength == 0.0) return color_in;

    color_in = clamp(color_in, 0.0, 1.0);

    vec3 color_hsv_in = rgb2hsv(color_in);

    //If there is an ambientlight, then the strangth has to be lowered:
    vec3 strength_vec3 = vec3(strength - ambilight);
    strength_vec3 = clamp(strength_vec3, 0.0,1.0);
    vec3 color_hsv_min = color_hsv_in;
    color_hsv_min.yz = scale_to_range_vec2(color_hsv_min.yz, -0.1, 0.1);

    vec3 color_rgb_min = hsv2rgb(color_hsv_min);


    vec3 pixel_out = mix(color_in, color_rgb_min, strength_vec3);

   //It could make sense to higher the contrast when ambient light hits the picture,
    //Maybe this could be used independently on the nightify feature?
    /*vec3 new_contrast = ambilight;
    vec3 new_brightness = ambilight*0.0;
    pixel_out = scale_to_range_vec3(pixel_out, -new_contrast, 1+new_contrast) + new_brightness;*/

    return pixel_out;
}


vec3 morph_shape_full_vec3(vec3 shape, vec3 l, float steep, float gamma ) {    
    vec3 l_pow = pow(l,vec3(gamma));
    vec3 l2 = min(l_pow * 16, steep);
    vec3 s1 = pow(shape, inversesqrt(l2));
    //vec3 s2 = (1-s1) * l_pow;
        //==  lpow - lpow * s1;
        //== -lpow * s1 + lpow;
    vec3 s2 = fma(-l_pow, s1,  l_pow); //ASM PROOF, faster.
    return (s2/(steep)) + s1;
}

vec3 morph_shape_full_no_steepness_vec3(vec3 shape, vec3 l, float gamma ) {
    vec3 l_pow = pow(l,vec3(gamma));
    vec3 l2 = min(l_pow * 16, 1);
    vec3 s1 = pow(shape, inversesqrt(l2));
    vec3 s2 = (1-s1) * l_pow;
        // == 1*l_pow  - s1*l_pow
        // == -l_pow*s1 + l_pow
    //vec3 s2 = fma(s1, -l_pow, l_pow); //ASM PROOF, slower.
    return s2 + s1;
}


vec3 morph_shape(vec3 shape, vec3 power, float steep, float gamma ) {
   return morph_shape_full_vec3(shape, power, steep, gamma);
}

vec3 downsample( sampler2D tex, vec2 uv, vec4 sourcesize,  float sharpness_add ) {
    vec2 sharpness = vec2(1.0)+sharpness_add;
    vec2 scale = sourcesize.xy * sharpness;

    vec2 iuv = floor(uv * scale);
    vec2 bottomleft  =    iuv;
    vec2 bottomright =  ( iuv + vec2(1.0,0.0)) / scale;
    vec2 topleft     =  ( iuv + vec2(0.0,1.0)) / scale;
    vec2 topright    =  ( iuv + vec2(1.0,1.0)) / scale;

    bottomleft /= scale;

    vec2 dist = (uv - bottomleft)*scale;

    vec3 bl = texture(tex, bottomleft).xyz   ;
    vec3 br = texture(tex, bottomright).xyz  ;
    vec3 tl = texture(tex, topleft).xyz      ;
    vec3 tr = texture(tex, topright).xyz     ;

    vec3 tA = mix( bl, br, dist.x );
    vec3 tB = mix( tl, tr, dist.x );
    return mix( tA, tB, dist.y );

}

vec3 downsample_x( sampler2D tex, vec2 uv, vec4 sourcesize, float sharpness_add ) {

    float sharpness = 1.0+sharpness_add;

    vec2 scale = vec2(sourcesize.x * sharpness, 1.0);

    vec2 iuv = vec2( floor(uv.x * scale.x), uv.y);

    vec2 bottomleft  =    iuv;
    vec2 bottomright =  ( iuv + vec2(1.0,0.0)) / scale;

    bottomleft /= scale;

    vec2 dist = (uv - bottomleft)*scale;

    vec3 bl = texture(tex, bottomleft).xyz   ;
    vec3 br = texture(tex, bottomright).xyz  ;

    return mix( bl, br, dist.x );

}


vec4 fn_pixel_grid(vec2 in_coords, vec4 pixel_in, float min_inLum, float max_inLum  ) {

    //Scanlines: compute each phosphor height according to input luminance, puttin it here helps parallelism a bit.
        vec3 phosphor_height = map_range(pixel_in.rgb, min_inLum, max_inLum, PIXELGRID_MIN_H, clamp(PIXELGRID_MAX_H, PIXELGRID_MIN_H, 1.0));

    //Tate ? (other outputsize,originalsize and outputcoords "tated" in vertex shader.)
        if (vDo_Tate == 1.0) in_coords.xy = in_coords.yx;

    //in_coords = in_coords - global.flick_and_noise_passSize.zw*0.5; //<-- needed if for whatever reason you want to draw scanlines and vgaps in first_pass

    //Get preset masks on ".rgb" and mask size on ".a" , mask and period for every phosphor done in vertex shader.
        vec3 PG_offsets = vPG_offsets_and_size.rgb;

        vec2 freq_base_core = in_coords * vPG_OriginalSize_tated_mul_pi ;

        //Use fake scanlines?
        if (vFake_scanline_period > 0.0)
            freq_base_core.y = vFake_scanline_period;

    //Screen coords:
    //Floor x screen coords when using non 1.0x sizes.
    //y screen coords appear to work just fine even if we don't floor them,
        vec2 freq_base_screen;
        if (PIXELGRID_MUL_X == 1.0) {
            freq_base_screen = vec2(vPG_freq_base_screen.x * floor(vPG_OutputCoord_tated.x * vPG_OutputSize_tated.x ), vPG_freq_base_screen_unfloored.y);
        } else {
            freq_base_screen = vPG_freq_base_screen_unfloored;
        }

    //Switch frequency between core and screen and apply asked multipliers
        vec2 freq_base = freq_base_core * vec2(vPG_period_multiplier_x, 1.0) ;
        vec2 freq_base_unfloored = freq_base_core;
        if (PIXELGRID_SIZE_W == 1.0) {
            freq_base.x = freq_base_screen.x * vPG_period_multiplier_x;
            freq_base_unfloored.x = vPG_freq_base_screen_unfloored.x * vPG_period_multiplier_x;
        }
    
    //shadowmask
    //works by staggering odd rows by 1.5X triad
    PG_offsets = vPG_offsets_and_size.rgb + //base offsets staggered (+)
                float( 
                       PIXELGRID_DO_SHADOWMASK *                                        // user wants shadowmask
                       sin(freq_base_screen.y * 2.0 / PIXELGRID_SHADOWMASK_H ) > 0.0   // sin > 0.0 means the row is even
                ) * PIXELGRID_SHADOWMASK_SHIFT ;                                                // stagger by 1.5

        
    vec3 freq_rgb = freq_base.xxx - PG_offsets;

    //The following is needed by scanlines and vertical mask
    //sin() tell use if the current cell position is odd or even:

        //FIXME: is_even should be a single phosphor property.
        //If i manage to find a way to do it, i could use
        //decon_stagger.r,g,b to stagger single phosphors.
        //And that way, i could blend staggered triads together
        //without "cuts" between them.
        //FIXME FIXME: Unsurer if the previous statement is still valid.

    //Moving it up there provides a small speed up due to
    //increased parallalism, I guess.
    
        float sin_check_offset = sin(freq_base_unfloored.x);
        float is_even = step(sin_check_offset, 0.0);


    //Scanlines and fake slotmask =========================================================================
        vec3 rgb_h = vec3(1.0);
        vec3 dedot_mix = vec3(0.0);

        //Pixelgrid: do horizontal if enabled, but skip it if user doesn't want it on interlaced content.
        //if (DO_PIXELGRID_H > 0.0 && !(PIXELGRID_INTR_DISABLE_Y==1.0 && vIsInterlaced==1.0)) { //103.1
        if (vDo_pixelgrid_h > 0.5) {
            //Scanline Inflation:
                #define inflation pixel_in.a
                freq_base_core.y += inflation;
            
            //Since we emulate scanlines, take into account current scanline phase:
                float interlacing_adapt_period = vScanlinePeriod;

            //We can offset trias to emulate fake core level slotmask by applying the optional offset to emulate the slotmask on even cells
                float fakeslot_stagger_offset =  is_even * PIXELGRID_OFFSET_CORE * pi;
            //get 3 sines with applied the optional staggered offset for slotmask, and single phosphors staggering for y deconvergence.
                vec3 decon_stagger = vec3( PIXELGRID_DECON_R_H, PIXELGRID_DECON_G_H, PIXELGRID_DECON_B_H);


        //Antimoire experiments:
            vec3 rgb_h_sin = vec3(1.0);
            float anti_inflation = 1-clamp(abs(pixel_in.a), 0.0,1.0); //<- counteract: deconvergence does not depend on inflaction 
            if (PIXELGRID_H_ANTIMOIRE > 0.0) {
                float anti_moire_freq = sin(vPG_freq_base_screen_unfloored.x * vPG_offsets_and_size.a );
                float anti_moire_amp = PIXELGRID_H_ANTIMOIRE;
                rgb_h_sin = sin( (freq_base_core.yyy * interlacing_adapt_period) + ( anti_moire_freq * anti_moire_amp ) + vInterlace_offset);
            } else {
                   rgb_h_sin = sin( (freq_base_core.y * interlacing_adapt_period) + fakeslot_stagger_offset - decon_stagger*anti_inflation + vInterlace_offset );
            }

            //make it positve with doubled frequency:
                rgb_h_sin = (rgb_h_sin * rgb_h_sin);

            //Compute dedot mix here for use in h mask and vmask2 later
                //dedot_mix = vec3(  PIXELGRID_H_DEDOT - PIXELGRID_H_DEDOT * rgb_h_sin.x);
                dedot_mix = vec3( fma( -PIXELGRID_H_DEDOT, rgb_h_sin.x, PIXELGRID_H_DEDOT)); //ASM PROOF: SAME
                dedot_mix = dedot_mix *(pixel_in.rgb/max_inLum);
                
            // Compute each phosphor height according to input luminance,
            // Moved outside the branch to help parallelism a bit
                //float extragain_h = max(PIXELGRID_MAX_H, 1.0); //Handle
                //vec3 phosphor_height = map_range(pixel_in.rgb*extragain_h, min_inLum, max_inLum, PIXELGRID_MIN_H, clamp(PIXELGRID_MAX_H, PIXELGRID_MIN_H, 1.0));


            //Finally get 3 sines out of the previous one by applying height modifiers
                rgb_h = morph_shape(rgb_h_sin, phosphor_height, PIXELGRID_NO_INTERBLEED_H, PIXELGRID_GAMMA_H);

                rgb_h = mix(vec3(1.0), rgb_h, DO_PIXELGRID_H);
                rgb_h = clamp(rgb_h, 0.0,1.0);

        }



    //Horizontal Triad Mask: ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        vec3 rgb_w = vec3(1.0);
        vec3 rgb_grille_basal = vec3(0.0);

        if (DO_PIXELGRID_W > 0.0) { //4%
            //get 3 sines in black/white
                rgb_w = sin(freq_rgb);
            //make them always positive and double frequency:
                rgb_w = (rgb_w * rgb_w);
            //Scale Max width according to pre-gains:
                vec3 phosphor_width = map_range(pixel_in.rgb, min_inLum, max_inLum, PIXELGRID_MIN_W, PIXELGRID_MAX_W);
            //Get final phosphor width:
                //rgb_w = morph_shape(rgb_w, phosphor_width, PIXELGRID_NO_INTERBLEED_W, PIXELGRID_GAMMA_W); //2%
                rgb_w = morph_shape_full_no_steepness_vec3(rgb_w, phosphor_width, PIXELGRID_GAMMA_W); //2%

            //Dedot rgb mask between h mask.
            //This work only when there are dots and have the countereffect when there are not.
            //This happen because flattening the rgb mask will make unflattened rgb triads
            //more evident, so this has to be used only when needed.
                rgb_w = mix(rgb_w, phosphor_width, dedot_mix);

            //lower strength via user parameter?
                rgb_w = mix(vec3(1.0), rgb_w, DO_PIXELGRID_W);
                
            //"export" naked vertical grille
                rgb_grille_basal = (rgb_w * rgb_w) * (rgb_w * rgb_w) * (PIXELGRID_BASAL_GRID*0.0025);
                
        }


    //Vertical mask .:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.:.
    // can be used for slotmask,aperturegrille ... or scanlines, but they would have a boxed shape.
        vec3 darklines_mask=vec3(1.0);

        vec3 pixel_in_unsparkled = pixel_in.rgb;
        if (PIXELGRID_Y_MASK > 0.0) {
            //Compute phosphors height mask with the right reference coords.
                float ph_mask_height;
                if ( PIXELGRID_Y_MASK_COORDS == 1.0 )
                    ph_mask_height = freq_base_screen.y * vPG_mask_height;
                else
                    ph_mask_height = freq_base_core.y * vPG_mask_height;

            //phosphors mask stagger
                #define PH_MASH_OFFSET  (PIXELGRID_Y_MASK_OFFSET * HALF_PI )

                
            //Use abs(cos) (later 1-abs(cos) to get horizontal gaps, use pow to make them steep.
            //Stagger them by PH_MASH_OFFSET on even triads
                float mask_shape = abs(cos(  vPG_y_shift  + ph_mask_height  + (PH_MASH_OFFSET * is_even)) ) ;
                mask_shape = pow(mask_shape, PIXELGRID_Y_MASK_STEEP);


                // We can make the stagger dark line less pronunced
                // on its sides so that lateral staggered color
                // can bleed over.
                // but you really need to have God eyes to notice any improvement, if any imho.
                // Also, this would probably alter the colors (?)
                // So keep it disabled.
                //    float Bleed_force = SERVICE1; //0.02 seems a realistic value.
                //    float side_visibility = pow(pow(sin_check_offset,2.0),Bleed_force);
                //    mask_shape = mask_shape* side_visibility;
                // return vec4(mask_shape);

            //Get per channel input luminance by clamping pixel_in and applying hmask
                vec3 lum = clamp(pixel_in.rgb * rgb_w, 0.0, 1.0);
            //Adapt the luminance effect via input parameter
                vec3 lum_adapted = (1-lum*PIXELGRID_Y_MASK_ON_WHITE);
            //Calc final mask visibility as the minimum between tha configured visibility and luminosity effect
                vec3 V=min( lum_adapted, vec3(PIXELGRID_Y_MASK) );
            //apply user visibility modifier to the mask
                darklines_mask = 1-(mask_shape*V);
            //Try to get a more brillant/sparkling look by highering the darklines
            //effect (sub) and pushing the source color, useful for slotmasks.
                vec3 PG_spark = (PIXELGRID_Y_SPARK * V) * (0.5/vPG_mask_height);         
                darklines_mask = (darklines_mask-0.5) * (1+PG_spark) + 0.5 ; 
        }


    //Finally put H mask, Scanlines and darklines masks together:
        vec3 mask = rgb_w * rgb_h * darklines_mask;


    float TEST_OVERMASK_NEW = 0.0;
    //Apply Overmask:
        if (TEST_OVERMASK_NEW == 1.0) {
            vec3 mask_overmask = PIXELGRID_OVERMASK * (mask - 0.5) + 0.5;
            vec3 pixel_in_overmask = mix(pixel_in.rgb, pixel_in.rgb * PIXELGRID_OVERMASK, PIXELGRID_OVERMASK);
        //Adaptive overmask experiment
            //different mix levels:
                mask = mix(mask, mask_overmask, abs(mask - vec3(0.5)) );
                pixel_in.rgb = mix(pixel_in.rgb, pixel_in_overmask, abs( pixel_in.rgb - vec3(0.5)  )      );
            //Same mix levels:
                //mask = mix(mask_overmask, mask,              pow( abs( pixel_in - vec3(0.5))*2.0, vec3(1.0))        );
                //pixel_in = mix(pixel_in_overmask, pixel_in,  pow(abs( pixel_in - vec3(0.5))*2.0, vec3(1.0))        );
            //Mix level applied to overmask:
                //vec3 overmask_adapted = mix(vec3(PIXELGRID_OVERMASK), vec3(1.0), abs(pixel_in - 0.5)*2.0 );
                //mask = overmask_adapted * (mask - 0.5) + 0.5;
                //pixel_in = mix(pixel_in, pixel_in * overmask_adapted, overmask_adapted);
        } else {
            mask = PIXELGRID_OVERMASK * (mask - 0.5) + 0.5;
            pixel_in.rgb = mix(pixel_in.rgb, pixel_in.rgb * PIXELGRID_OVERMASK, PIXELGRID_OVERMASK);
        }
    //Apply the mask to pixel_in and clamp the minimum to the unexcited grille.
        vec3 mask_and_grille = max(mask * pixel_in.rgb, rgb_grille_basal);

    //return it with alpha channel containing the mask itself, so that halo can selectively light it.
        return vec4( mask_and_grille , dot(rgb_h, vec3(0.3333)));
}


vec4 fn_pixel_dotmat(vec2 coords, vec3 pixel_in) {

    //Base angle, depends on original size:
        vec2 angle_base = coords * pi * params.OriginalSize.xy; //FIXME here we want .yy to make integer scaling

    //Zoom to debug:
        angle_base/=DOT_M_MULTIPLY;

    //Set the grid sharpness from vertex shader:
        float s_grid = vDotMat_Grid_Sharpness;


    //Generate bw grid:
        vec2 grid = cos(angle_base);
        grid = grid * grid;
        grid = pow(grid, vec2(s_grid));
        grid = 1 - grid;


    //Combine gridx and gridy into a single one
        //float fgrid = min(grid.x, grid.y);
        float fgrid = grid.x* grid.y;

    //Modify strength for "paint on background" option

        //Smoothly fadeout grid on background
            //The fadeout size:
                float fade_size = 0.3;

            //We invert the smooth logic if threshold is negative. this allow to fadeout
            //the grid on bright or dark backgrounds.
                float lum_scaled = dot(pixel_in, vec3(0.33333));
                lum_scaled = clamp(lum_scaled, 0.0, 1.0);
                lum_scaled = mix_step(1-lum_scaled, lum_scaled, float(DOT_M_G_TRESH > 0.0));

            float smooth_threshold = abs(DOT_M_G_TRESH);
            vec2 smooth_range  = vec2(smooth_threshold, smooth_threshold+fade_size);
            float grid_smoothstep = 1- smoothstep(smooth_range.x, smooth_range.y, lum_scaled);

    //Adapt grid strength
        float grid_str = DOT_M_G_STR * grid_smoothstep;
    //Apply strength modifier
        float fgrid_adpt = mix(1.0, fgrid, grid_str);

    //Output is the mix of the gap and the rgb masked pixel in.
    //In the alpha channel we put the grid because it could be selectiveli brightened by halo
    return vec4(
                 mix(vDotMat_Grid_Color, pixel_in, fgrid_adpt),
                 fgrid_adpt
               );

}

float get_clamped_white_reference(vec3 pixel_in){
    float white_reference = max(max(pixel_in.r,pixel_in.g),pixel_in.b);
//     white_reference = min(white_reference,1.0); //Clamp here for both vmask and darklines.
    return white_reference;
}

vec3 pixel_backdrop_image(float isrotated) {
    vec2 backdrop_offset=vec2(BACKDROP_OFFX,BACKDROP_OFFY);

    vec2 backdrop_tex_size = textureSize(backdrop, 0);
    float backdrop_lod = log2(backdrop_tex_size.y * global.FinalViewportSize.w);

    vec2 backdrop_coords =  get_scaled_coords_aspect(
                            vOutputCoord+backdrop_offset,
                            global.FinalViewportSize,
                            backdrop_tex_size.x/backdrop_tex_size.y,
                            bool(isrotated));

    backdrop_coords=zoom(backdrop_coords, BACKDROP_ZOOM);
    return textureLod(backdrop, backdrop_coords, backdrop_lod).rgb;
}


vec4 textureLod_wrap(sampler2D tex, vec2 co, float lod, float wrap_mode) {
    #ifdef ALLOW_BG_IMAGE_TEXTURE_WRAP_IN_SHADER
        /*
        // Mirrored repeat, once, useless since is done by default
        if (co.x > 1.0 || co.x < 0.0)
            co.x = 1- mod(co.x, 1.0);
        if (co.y > 1.0 || co.y < 0.0)
            co.y = 1- mod(co.y, 1.0);
        */

        if (wrap_mode == 1.0) {
            //Clamp to border, black.
            //bool bOutside = (co.x < 0.0 || co.x > 1.0 || co.y < 0.0 || co.y > 1.0 ) ;
            //if (bOutside) return vec4(0.0,0.0,0.0,1.0);
            
            if (is_first_outside_rect(co, vec4(0.0,0.0,1.0,1.0))) //ASM PROOF, FASTER.
                return vec4(0.0,0.0,0.0,1.0);

        } else if (wrap_mode == 2.0) {
            //Clamp to edge:
            co = clamp(co, 0.00, 1.0);
        } else if (wrap_mode == 3.0) {
            //Repeat no mirror:
            co = mod(co, 1.0);
        }
    #endif
    return textureLod(tex, co, lod);
}



/*vec3 pixel_blank_alternate_v2(vec2 co, vec4 texsize, float strength, vec3 c) {

	c = pow(c, vec3(2.2));

    vec3 S=vec3(strength);
    vec3 ref = clamp(c, 0.0,1.0);
    S = min(S, vec3(1.0)-ref);
    S = min(S, ref);

    float line = co.y * texsize.y;
    float column = co.x * texsize.x;

    float l_period_half = ALT_BLANK_PERIOD / 2;

    float sinlinea= sin(line*pi/ALT_BLANK_PERIOD);
    sinlinea*=sinlinea;

    float sincolonna= sin(column*pi/ALT_BLANK_PERIOD);
    sincolonna*=sincolonna;


    bool linea_pari = sinlinea > 0.5;
    bool frame_pari =  mod(float(params.FrameCount),2.0  ) == 1 ;

    if (frame_pari) {
        if (linea_pari)
            S = -S;
                else
            S = +S;
	} else {

		if (!frame_pari) {
			if (linea_pari)
				S = S;
					else
				S = -S;
		}
    }

    c = c+S;

	c = pow(c, vec3(1/2.2));

    return c;
}


vec3 pixel_blank_alternate(vec3 strength, vec3 pixel_in) {
    //strength *= 1-pixel_in; <<--- less on bright areas?
    strength =  1-strength;

    float square_wave_line = float(mod((vTexCoord.y+0.0001) * params.OutputSize.y, ALT_BLANK_PERIOD) > ALT_BLANK_PERIOD / 2);
    if ( mod(params.FrameCount, 2.0) == 1.0) {
        return min(square_wave_line + strength, vec3(1.0));
    } else {
        return min(1 - square_wave_line + strength, vec3(1.0));
    }

}
*/

vec3 bezel_color(float lum)  {

    //Colorize bezel frame
    vec3 col = vec3(BEZEL_R,BEZEL_G,BEZEL_B) + lum;
    float con_max = fma(0.5, BEZEL_CON, 0.5); // <<- same as: float con_max =  0.5 * BEZEL_CON + 0.5;
    col = scale_to_range_vec3(col, -con_max+1, con_max);
    return clamp(col,0.0,1.0);

}


float fuzzyrect(vec2 uv, vec2 size, float radius, float blur) {
    vec2 hSize = size / 2.0 - radius;
    float d = length(max(abs(uv - vec2(0.5)),hSize)-hSize);
    //return smoothstep(-radius-blur, -radius+blur, -d);
    return smoothstep_fast(-radius-blur, -radius+blur, -d);
}


float create_ambi_colorize_shade(vec2 co) {
    float blur = AMBI_OVER_BEZEL_SIZE;
    vec2 size = vec2(1.0, 2 - AMBI_OVER_BEZEL_AR_CORRECTION)-blur;
    float radius = 0.0;

    return  1 - min ( fuzzyrect(co, size,  radius,  blur) * 2, 1.0);
}

vec2 get_scaled_coords_for_bezel(vec2 co, float isrotated) {
    //This function is here because compiler gets mad if i calc coords_for_bezel
    //outside the main branch "if DO_BEZEL then compose_bezel_over"
    //performances falls down for no apparent reason.
    //But still, i need to access it twice in the code.
    //So this is a function that ensures me that i always calc it the same way.

    co = zoomout_coords(co, -BEZEL_FRAME_ZOOM); //FIXME COULD BE FASTER.
    co.y = zoom1D(co.y, BEZEL_ASPECT_CORRECTION );

    if (DO_TILT == 1.0)
        return tilt(co, isrotated, TILT_X * TILT_BEZEL_K);
            else
        return co;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


vec4 fn_pixel_fgbg_image(sampler2D smp) {
    vec2 tex_size = textureSize(smp, 0); // * BG_ZOOM;
    //if (BG_IMAGE_ROTATION > 0.0  || ( BG_IMAGE_ROTATION < 0.0 && bool(vIsRotated) )  ) tex_size.xy = tex_size.yx;

    //FIXME isrotated deve essere true anche quando l'immagine di sfondo è ruotata.
    //vec2 fg_image_offset=vec2(BG_IMAGE_OFFX,BG_IMAGE_OFFY);
    //back_coords = get_scaled_coords_aspect(vBg_img_coords + fg_image_offset, global.FinalViewportSize, tex_size.x/tex_size.y, bool(vIsRotated));

    //bool rotated = bool(vIsRotated) || BG_IMAGE_ROTATION > 4.5;
    bool rotated = BG_IMAGE_ROTATION > 3.5;

    vec2 back_coords = get_scaled_coords_aspect(vBg_img_coords, global.FinalViewportSize, tex_size.x/tex_size.y, rotated);


    float bg_over_lod = log2(tex_size.y * global.FinalViewportSize.w);
    vec4 pixel_bgover = textureLod_wrap(smp, back_coords, bg_over_lod, BG_IMAGE_WRAP_MODE);
    return pixel_bgover;
}

vec3 light_over_image(vec3 light, vec3 image, float black_illumination) {
    //Simulates illumination.
    //It works by adding the light on the image.
    //It will add less light on dark colors (

    //mitigate clipping by lowering light on bright images:
    //light = light * (1 - max(max(image.r,image.g),image.b) )   ; //1313/20079
    //light =   light - light*max(max(image.r,image.g),image.b); //ASM PROOF FASTER 1310 20046
    //light = -light * max(max(image.r,image.g),image.b) + light ; //ASM PROOF FASTER
    light = fma(vec3(max(max(image.r,image.g),image.b)),
            -light, 
            light );

    vec3 light_on_black = black_illumination * light;
    vec3 modulated_on_black = image.rgb +  ( light  * image.rgb ) + light_on_black;


    return modulated_on_black;
}

vec3 ambi_blend_image(vec4 image, vec3 ambi, float blend_mode) {
    //mix or add ambient light with image, also allow force colorization in add mode.
    if (DO_AMBILIGHT == 0.0) return image.rgb;

    // Fake a transparent image when force colorization is requested
    // So that we can use the same code used for alpha blend.
    // Also multiply AMBI_BG_IMAGE_FORCE * AMBI_BG_IMAGE_BLEND_MODE to skip
    // force colorization when mode blend mode is not "ADD".
        float image_alpha_adapted = image.a - (AMBI_BG_IMAGE_FORCE * AMBI_BG_IMAGE_BLEND_MODE);

    if (AMBI_BG_IMAGE_BLEND_MODE == 0.0) {
        image.rgb = mix(ambi.rgb, image.rgb, image_alpha_adapted);
    } else {
        //vec3 ambi_alpha_masked = ambi.rgb *  (1 -image_alpha_adapted);
            // (ambi.rgb * -image_alpha_adapted) + (ambi * 1)
        vec3 ambi_alpha_masked = fma(ambi.rgb, vec3(-image_alpha_adapted),  ambi.rgb) ; //ASM PROOF: FASTER
        image.rgb = light_over_image(ambi_alpha_masked, image.rgb, AMBI_ADD_ON_BLACK);
    }
    return image.rgb;
}

float gaussian_coefficient(float x, float sigma) {
    //restituisce un coefficiente gaussiano per x compreso tra 0 ed 1
    float coefficient = 1.0 / sqrt(2.0 * 3.14159265358979323846 * sigma);
    float exponent = -((x * x) / (2.0 * sigma));
    return coefficient * exp(exponent);
}

vec3 fn_pixel_content(vec2 coords) {

    vec3 pixel_out;
    vec4 pixel_glowed;
    float dot_mat_or_pixelgrid = 1.0; //init dot grid or scanline mask to 1.0


    //#define USE_QUILEZ

    #ifdef USE_QUILEZ
        pixel_glowed = texture(in_glow_pass, coords_QULEZ(coords, global.flick_and_noise_passSize));
        pixel_out = pixel_glowed.rgb;
    #else
        pixel_glowed = texture(in_glow_pass, coords);
        pixel_out = pixel_glowed.rgb;
    #endif        
    //Pixel grid
        if (DO_PIXELGRID == 1.0) {
            float vMax_inLum = max( 1.0, DO_CCORRECTION * apply_contrast_brightness(1.0, CONTRAST, BRIGHTNESS)) *
                               max( 1.0, mix(1.0, IN_GLOW_POWER, DO_CCORRECTION));
            vec4 pixel_grid = fn_pixel_grid(coords, pixel_glowed, 0.0, vMax_inLum);
            //grid mask only is needed by halo to selectively light the grid.
                dot_mat_or_pixelgrid = pixel_grid.a;
                pixel_out = pixel_grid.rgb;
        }
    //Dot mask
        if (DO_DOT_MATRIX == 1.0) {
            vec4 pixel_dotmat = fn_pixel_dotmat(coords, pixel_out);
            //grid mask only is needed by halo to selectively light the grid.
                dot_mat_or_pixelgrid = pixel_dotmat.a;
            //rgb channel goes to pixel_out
            pixel_out = pixel_dotmat.rgb;

        }

    //Halo
        if (DO_HALO == 1.0 ) {
            //vec2 cq = coords_QULEZ(coords, global.in_glow_passSize);
            //vec2 ch = mix(coords, cq, SERVICE1);
            //pixel_haloed = texture(halo_pass,ch).rgb;

            vec2 halo_coords = coords;

            if (HALO_PRE_SCANLINE_GAP == 1.0 &&  vDo_pixelgrid_h == 1.0) {
                //FIXME dot_mat lasciato indietro
                halo_coords.y += (DO_PIXELGRID_H  * global.in_glow_passSize.w) * ( 0.5 * (0.75-HALO_VS_SCAN) );
                //halo_coords.y +=  (DO_PIXELGRID_H  * global.in_glow_passSize.w) * ( 0.375-0.5*HALO_VS_SCAN ); //ASM PROOF SLOWER
                vec3 pixel_haloed = texture(halo_pass, halo_coords).rgb;

                //pixel_haloed = blur_px(halo_pre_gamma_pass, halo_coords, vec2(SERVICE1*0.1)) ;

                pixel_out += pixel_haloed;
            } else {
                vec3 pixel_haloed = texture(halo_pass, halo_coords).rgb;
                //Halo only on scanlines:
                pixel_out += pixel_haloed * dot_mat_or_pixelgrid ;
                //Halo over scanlines gap too:
                pixel_out += (pixel_haloed * HALO_VS_SCAN) * (1 - dot_mat_or_pixelgrid) ;
            }

        }

    // Apply gamma out:
        if (DO_CCORRECTION == 1.0)
            pixel_out = pow(max(pixel_out, vec3(0.0)),vec3(GAMMA_OUT));

    //Bloom
    /*    if (DO_BLOOM == 1.0 ) {
            vec3 bloomed=texture(bloom_pass_final, coords).rgb ;
            if (BLOOM_BYPASS != 1.0 ) {
                vec3 lum = clamp(pixel_glowed, 0.0,1.0);
                vec3 darkness=(1-lum) * (1-lum);
                bloomed = mix( bloomed * darkness , bloomed, BLOOM_OVER_WHITE);
            }
            pixel_out = bloomed + float(BLOOM_BYPASS < 0.5) * pixel_out;
        }*/
    //Bloom
        if (DO_BLOOM == 1.0 ) {
            vec3 bloomed=texture(bloom_pass_final, coords).rgb ;
            pixel_out = float(BLOOM_BYPASS < 0.5) * pixel_out + bloomed;
        }


    return pixel_out;
}


vec4 fn_pixel_bezel(vec2 coords_for_bezel, vec2 coords_for_mirror, float nightify_str, vec3 pixel_ambilight) {

    //Can we skip Blank outside border and inner blank tube?
        vec4 inner_blank_rect = vec4(1-BEZEL_TUBE_BLANK_SIZE, 1-BEZEL_TUBE_BLANK_SIZE, BEZEL_TUBE_BLANK_SIZE, BEZEL_TUBE_BLANK_SIZE);
        if (is_first_outside_rect(coords_for_bezel, RECT01)  ||
            is_first_inside_rect(coords_for_bezel, inner_blank_rect)
        ) return vec4(0.0);

    vec4 bezel_in;

    //Sample main bezel texture:
    //We assume straight and curved textures are the same size. WARNING
        #ifndef BEZEL_RES
            vec2 bezel_lut_size = textureSize(monitor_body_curved, 0);
            float bezel_frame_lod = log2(bezel_lut_size.y * (BEZEL_FRAME_ZOOM+1.0) * global.FinalViewportSize.w);
        #else
            //float bezel_frame_lod = log2(    fma( BEZEL_RES.y,  BEZEL_FRAME_ZOOM,  BEZEL_RES.y) * global.FinalViewportSize.w); //ASM PROOF: UNDECIDED
            float bezel_frame_lod = log2( BEZEL_RES.y * (BEZEL_FRAME_ZOOM+1.0) * global.FinalViewportSize.w); 
        #endif

        if (BEZEL_USE_STRAIGHT < 0.5)
            bezel_in = textureLod(monitor_body_curved,coords_for_bezel,bezel_frame_lod);
                else
            bezel_in = textureLod(monitor_body_straight,coords_for_bezel,bezel_frame_lod);


    //Exit if the bezel is completely transparent (the tube)
        if (bezel_in.a == 0.0) return vec4(0.0);

    //Colorize the bezel (bezel.r expresses the luminance)
        vec3 bezel_out = bezel_color(bezel_in.r);

    float lut_specular = bezel_in.g * BEZEL_SPCL_STRENGTH;
    float reflection_modifier = 0.0;
    vec4 pixel_mirrored = vec4(0.0);

    // Calculate Reflections, can be skipped if blue channel is 0:
        if (bezel_in.b > 0.0) {
            
        //Reflections: Calculate the bezel roughness to apply to the reflecting area. (bezel_in.g expresses the specularity)
            #define ROUGHNESS_ADAPT (2/1080.0 * BEZEL_ROUGHNESS)
            float roughness = (1 - min(lut_specular * 10, 1.0)) * //Skip drawing roughness over specular, it looks bad
                              (random_fast(vTexCoord) * ROUGHNESS_ADAPT) - ROUGHNESS_ADAPT*0.5;
                
            //Reflections:
                reflection_modifier = bezel_in.b; //bezel_in.b expresses how much the area is reflective

            //Sample the reflection pass with small offset to simulate the roughness
                pixel_mirrored = texture(reflected_blurred_pass, coords_for_mirror + roughness);
                float fcorners_shade = 1 - corners_shade(coords_for_bezel, 1.0) * BEZEL_CORNER_DARK;

            //Show less reflections in the corners
                pixel_mirrored.rgb *= fcorners_shade;

            //Push it over the specular areas and apply the reflection modifier
                //pixel_mirrored.rgb = pixel_mirrored.rgb * (1 + lut_specular);
                //pixel_mirrored.rgb = pixel_mirrored.rgb + pixel_mirrored.rgb * lut_specular; //ASM PROOF: FASTER
                pixel_mirrored.rgb = fma( pixel_mirrored.rgb, vec3(lut_specular), +pixel_mirrored.rgb); //ASM PROOF: FASTER
                
        }

    //Yes, we already passed ambilight as parameter, but if geometry content is overridden
    //then ambientlight may have not been sampled yet due to some skip logic made in the main()
        if (DO_AMBILIGHT == 1.0) 
            pixel_ambilight = texture(ambi_temporal_pass, vOutputCoord).rgb;

    //Apply nightification, the strength is modulated by ambient light.
        //Use a shaded box to apply ambilight on external borders only
            float ambi_colorize_shade = create_ambi_colorize_shade(coords_for_bezel);


            vec3 ambi_over_bezel = (AMBI_OVER_BEZEL * pixel_ambilight) *
                                   (bezel_in.a * ambi_colorize_shade) *
                                   (1-reflection_modifier) ;

        bezel_out.rgb = fn_pixel_nightify(bezel_out.rgb, nightify_str, ambi_over_bezel )   ;


    //Add reflections after nightification
            bezel_out += (pixel_mirrored.rgb * reflection_modifier);

        //Apply ambient light over the bezel
            bezel_out = light_over_image(ambi_over_bezel, bezel_out, AMBI_ADD_ON_BLACK);

    //Diffuse the light over specular areas, we use a mipmap with low precision.
        if (lut_specular > 0.0) {
            vec4 pixel_avglum = texture(avglum_pass, coords_for_mirror);
            bezel_out = bezel_out + (pixel_avglum.rgb * lut_specular) ;
        }

    return vec4(bezel_out, bezel_in.a);
}


void main() {
    //Initial content coords
        vec2 co_content = vTexCoord;

    //Tilt?
        if (DO_TILT == 1.0)
            co_content = tilt(co_content, vIsRotated, TILT_X);


    //Precalc Bezel coords, since it modifies content coords.
        vec2 co_bezel = vec2(0.0);
        if (DO_BEZEL == 1.0) {
            co_content = zoomout_coords(co_content, -vBEZEL_INNER_ZOOM_adapted);
            co_bezel = get_scaled_coords_for_bezel(vTexCoord, vIsRotated);
        }


    //Curvature
        //Curvature has to come after inner zoom or bezel curved border will not match content
        //curved border when inner zoom changes.
        if ( curvature_needed() > 0.0 ) {
            co_content = Warp_koko(co_content, vec2(GEOM_WARP_X, GEOM_WARP_Y), 0.5);
        }

    //Mirror coords needs to be calculated here, before geom override, but after curvature.
    //It is still not perfect but a reasonable tradeoff by now.
        vec2 co_mirror = zoom(co_content, 1/BEZEL_REFL_ZOOMOUT_ROOM);
        


  /*  float anaborderstart = 0.5;
    float anabordersize = anaborderstart + SERVICE1;
    float side_zoom = 1-smoothstep_fast(anaborderstart-0.5, anabordersize-0.5,abs(co_content.x-0.5)) ;
    co_content.x = zoom1D(co_content.x, 1/side_zoom);
    //FragColor = vec4(side_zoom) ; return;
    */
    
    float canvas_busy = 0.0; //<-- this allow for paint over not painted areas (spares gpu cycles)
    //Create an alpha mask to write content into, it holds opacity info that will be used to compose:
        if (DO_CURVATURE == 1.0)  {
            canvas_busy = fn_border(co_content);
        } else {
            canvas_busy = float(is_first_inside_rect(co_content, RECT01)); //FIXME: is step() faster?
        }
        

    //Apply other content coords modifiers
        if (DO_GAME_GEOM_OVERRIDE == 1.0)
            co_content = content_geom_override_zoom(co_content, GAME_GEOM_ASPECT, vIn_aspect, GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM);
 
    #ifndef D3D_WORKAROUND
        if (DELTA_RENDER == 1.0) {
            bool reuse_old =  bool( float( texture(in_glow_pass, co_content).a == 0.0) * canvas_busy * vDeltaRenderOk ) ;
            if (reuse_old) {
                FragColor = texture(final_passFeedback, vOutputCoord);// * vec4(1,0,0,0);
                return;
            }
        }
    #endif
    
    
        //Dynamic lum dependant full screen zoom?
    #ifndef D3D_WORKAROUND
        if (DO_DYNZOOM > 0.0)
            co_content = zoom(co_content, get_dyn_zoom(avglum_pass) );
    
        if (AUTOCROP_MAX > 0.0)
            co_content = zoom(co_content, get_autocrop_zoom(avglum_pass) );
    #endif





    vec3 pixel_ambi = vec3(0.0);
    vec3 pixel_under_content = vec3(0.0);
    vec3 pixel_out = vec3(0.0);
    
    //Draw content only over the alpha mask, and sample ambientlight outside it to spare gpu cycles.
        if (canvas_busy > 0.5) {
            pixel_out = fn_pixel_content(co_content) * canvas_busy;
            
            //pixel_out = adaptive_strobe_checkerboard(pixel_out, vOutputCoord, global.FinalViewportSize.xy);
            
            #ifndef D3D_WORKAROUND
                if (LCD_ANTIGHOSTING > 0.0) {
                    vec3 feedback = texture(final_passFeedback, vOutputCoord).rgb;
                    vec3 diff=(feedback - pixel_out);
                    pixel_out = mix(pixel_out, pixel_out-diff, LCD_ANTIGHOSTING );
                }
            #endif

        } else {

            //Can we halve refresh rate in the outside border to spare some cycles?
            #ifndef D3D_WORKAROUND
                if ( HALVE_BORDER_UPDATE == 1.0 ) {
                    if ((params.FrameCount % 2) != 1.0) {
                        pixel_out = texture(final_passFeedback, vOutputCoord).rgb;
                        FragColor = vec4(pixel_out, 1.0);
                        return;
                    }
                }
            #endif

            //Ambient light
            if (DO_AMBILIGHT == 1.0) {
                pixel_ambi = texture(ambi_temporal_pass, vOutputCoord).rgb ;
                
                if (DO_BG_IMAGE != 1.0) {
                    const float noise_power = NOISEPOWER * NOISEPOWER_AMBI_MUL;
                    float dynamicseed = mod(params.FrameCount, 30.0001);
                    float anoise1 = random_fast(vTexCoord*dynamicseed);
                    anoise1 = scale_to_range(anoise1, -noise_power, noise_power);
                    // anoise1 *=  (1 - pixel_ambi.g);
                    //anoise1 =  (anoise1 - anoise1 * pixel_ambi.g); //ASM PROOF FASTER
                    anoise1 =  fma(-anoise1, pixel_ambi.g, anoise1); //ASM PROOF FASTER
                    
                    pixel_ambi += anoise1;
                }
                //pixel_under_content = clamp(pixel_ambi, 0.0,1.0) * (1-canvas_busy); //<-- this one is better because will antialias borders
                                                                                      //    better (via canvas-busy) when ambientlight > 0.0
                //pixel_under_content = pixel_ambi * (1-canvas_busy);
                //pixel_under_content = pixel_ambi - canvas_busy * pixel_ambi; //ASM PROOF: FASTER
                pixel_under_content = fma(vec3(-canvas_busy), pixel_ambi, pixel_ambi); //ASM PROOF: FASTER
            }
        }

    //This represents the inner bezel alpha (when using bezel)
    //or the pixel_content canvas (when not using bezel)
    //It is used to draw spot on the whole inner tube even when toe content does not
    //fill the whole tube.
        float canvas_bezel_screen = canvas_busy;

    //Draw Bezel
        vec4 pixel_bezel;
        //SKIP LOGIC is inside fn_pixel_bezel
        if (DO_BEZEL == 1.0) {
            pixel_bezel = fn_pixel_bezel(co_bezel, co_mirror, BG_IMAGE_NIGHTIFY * DO_BG_IMAGE, pixel_ambi);
            //If we used a smooth_border, canvas_busy is it, but since the content is in the bezel,
            //we can safely use it to smooth/darken the game border
            pixel_out *= canvas_busy;
            pixel_out = mix(pixel_out, pixel_bezel.rgb, pixel_bezel.a);

            //Update alpha mask.
            //We can't use the bezel alpha channel to update the alpha mask since it is transparent ù
            //on the tube and we don't want to paint anything there, so use a dumb rect and add it to canvas_busy:
                float rect_bezel = float(is_first_inside_rect(co_bezel, vec4(BEZEL_FRAME_SURROUND_MARGIN, BEZEL_FRAME_SURROUND_MARGIN, 1.0 - BEZEL_FRAME_SURROUND_MARGIN, 1.0 - BEZEL_FRAME_SURROUND_MARGIN)));
                canvas_busy = max(pixel_bezel.a, rect_bezel);

                //FragColor = vec4(canvas_busy); return; // <- uncomment to debug BEZEL_FRAME_SURROUND_MARGIN

                //Intersect rect_bezel and bezel alpha to get the inner tube alpha
                canvas_bezel_screen = (1 - pixel_bezel.a) * rect_bezel;
                //canvas_bezel_screen = rect_bezel - rect_bezel* pixel_bezel.a; //ASM PROOF: SLOWER
        }

    if (canvas_bezel_screen > 0.5) {

        //Vignette
            if (DO_VIGNETTE == 1.0) {
                float spot_vignette_noise = random_fast(vTexCoord);
                spot_vignette_noise = scale_to_range(spot_vignette_noise, -NOISEPOWER, NOISEPOWER);
                vec2 sinco = cos((co_content-0.5) / V_SIZE) ;
                pixel_out = (pixel_out * V_POWER) * (sinco.y * sinco.x) + spot_vignette_noise;
            }

        //Spot
            if (DO_SPOT == 1.0) {
                    float dist = length( vec2(  (co_content.x-0.5) * vIn_aspect, co_content.y-0.5) +
                                         vec2(   S_POSITION_X      * vIn_aspect, S_POSITION_Y) );
                    //pixel_out += smoothstep_fast(S_SIZE,0.0,dist) * (S_POWER * canvas_bezel_screen); //ASM PROOF FASTER
                    //pixel_out += smoothstep_cos(S_SIZE,0.0,dist) * (S_POWER * canvas_bezel_screen); //ASM PROOF MORE INSTRUCTIONS, LESS CYCLES
                    //pixel_out += smoothstep(S_SIZE,0.0,dist) * (S_POWER * canvas_bezel_screen);
                    pixel_out += smoothstep_fast(S_SIZE * 0.90 ,0.0,dist) * (S_POWER * 1.125 * canvas_bezel_screen);
                    //pixel_out = mix(pixel_out, vec3(1.0), smoothstep(S_SIZE,0.0,dist) * S_POWER * canvas_bezel_screen); //<-- this is a bit slower, but more realistic.
            }
    }
        //Background image have, no need to paint if not in the outer border:
            if (DO_BG_IMAGE == 1.0 && BG_IMAGE_OVER == 0.0 && canvas_busy < 1.0) {
                vec4 pixel_bg_image = fn_pixel_fgbg_image(bg_under);

                                                        //pixel_bg_image.rgb = scale_to_range_vec3(pixel_bg_image.rgb, -pixel_ambi, 1+pixel_ambi);
                vec3 anti_nightify_ambi = (pixel_ambi * AMBI_BG_IMAGE_BLEND_MODE) *
                                           max(AMBI_BG_IMAGE_FORCE, 1-pixel_bg_image.a) ;


                pixel_bg_image.rgb = fn_pixel_nightify(pixel_bg_image.rgb, BG_IMAGE_NIGHTIFY, anti_nightify_ambi );
                pixel_bg_image.rgb = ambi_blend_image(pixel_bg_image, pixel_ambi, AMBI_BG_IMAGE_BLEND_MODE);

                pixel_under_content = pixel_bg_image.rgb;
            }


        //Smooth the image corners: canvas_busy (the alpha mask) is a white rect with shaded borders.
            pixel_out = mix (pixel_under_content, pixel_out, canvas_busy);


        //Backdrop
            if (DO_BACKDROP == 1.0)
                pixel_out += pixel_backdrop_image(vIsRotated);


        //Foreground image
            if (DO_BG_IMAGE + BG_IMAGE_OVER == 2.0) {
                vec4 pixel_fg_image = fn_pixel_fgbg_image(bg_over);
                // if geometry content is overridden, ambientlight may have not been sample yet due to some skip logic
                if (DO_AMBILIGHT == 1.0 && (pixel_fg_image.a+AMBI_BG_IMAGE_FORCE) > 0.0  && canvas_busy < 0.01) {
                    pixel_ambi = texture(ambi_temporal_pass, vOutputCoord).rgb * (1- canvas_busy ); //ALREADY TRIED COMMON fma() optimization, failed.
                }
                vec3 anti_nightify_ambi = (pixel_ambi * AMBI_BG_IMAGE_BLEND_MODE) *
                                           max(AMBI_BG_IMAGE_FORCE, 1-pixel_fg_image.a) ;

                pixel_fg_image.rgb = fn_pixel_nightify(pixel_fg_image.rgb, BG_IMAGE_NIGHTIFY, anti_nightify_ambi );

                // tl:dr:
                //   dont "black" the transparent areas outside the game content when we use AMBI_BG_IMAGE_BLEND_MODE ADD blend mode.
                //   Do only for add mode, since some inplace presets relies on the blackness with AMBI_BG_IMAGE_BLEND_MODE mix mode.
                // long explaination:
                //   We want to show the underlying game content when the image is transparent.
                //   This mean that alpha channel reveals the underlying content, but doing that even
                //   when the underlyinig content is void is not only pointless, but even counterproductive.
                //   Think at when we want to use ambient lights to *light* the part of the image
                //   that have a low alpha channel, that part would be black.
                if (DO_AMBILIGHT + AMBI_BG_IMAGE_BLEND_MODE == 2.0) {
                    float ambi_mask = create_ambi_colorize_shade(co_bezel);
                    float alpha_selective = clamp(  pixel_fg_image.a + (1- canvas_busy) , 0.0, 1.0); 
                    pixel_out = mix(pixel_out, pixel_fg_image.rgb, alpha_selective );
                    float fg_image_alpha_adapted = max(pixel_fg_image.a - AMBI_BG_IMAGE_FORCE, 0.0);
                    vec3 light = pixel_ambi.rgb * (ambi_mask) * (1- fg_image_alpha_adapted);
                    pixel_out = light_over_image(light, pixel_out, AMBI_ADD_ON_BLACK);
                } else {
                    pixel_out = mix(pixel_out, pixel_fg_image.rgb, pixel_fg_image.a);
                }
            }


    //Debug functions:
        #ifdef DEBUG_QUAD_SPLIT
            if ( vOutputCoord.x < 0.5  && vOutputCoord.y > 0.5 || vOutputCoord.x > 0.5  && vOutputCoord.y < 0.5 )
            pixel_out = texture(DEBUG_TEXTURE,vOutputCoord).rgb;
        #endif

        #ifdef DEBUG_DUAL_SPLIT_Y
            if (vTexCoord.y < 0.5) pixel_out = texture(DEBUG_TEXTURE,vTexCoord).rgb;
        #endif

        #ifdef DEBUG_DUAL_SPLIT_Y_GAMMA
            if (vTexCoord.y < 0.5) pixel_out = pow(texture(DEBUG_TEXTURE,vTexCoord).rgb, vec3(1.08));
        #endif

        #ifdef DEBUG_DUAL_SPLIT_X
            if (vTexCoord.x > 0.5) pixel_out = texture(DEBUG_TEXTURE,vTexCoord).rgb;
        #endif

        #ifdef DEBUG_DUAL_SPLIT_MIRROR_X
            if (vTexCoord.x > 0.5) pixel_out = texture(DEBUG_TEXTURE,vec2(1-vTexCoord.x, vTexCoord.y) ).rgb;
        #endif

        #ifdef DEBUG_DUAL_SPLIT_MIRROR_X_GAMMA
            if (vTexCoord.x > 0.5) pixel_out = pow(texture(DEBUG_TEXTURE,vec2(1-vTexCoord.x, vTexCoord.y) ).rgb, vec3(1.11));
        #endif

        #ifdef DEBUG_PRINT_VALUE
            float maxdigits = 10.0;
            float decimalplaces = 2.0;
            float fvalue = vfprintvalue;
            vec2 vFragCoord = vec2( floor(vOutputCoord.x * params.OutputSize.x),
                                floor(vOutputCoord.y * params.OutputSize.y));
            pixel_out += PrintValueVec3( vTexCoord, vFragCoord, fvalue, maxdigits, decimalplaces );
        #endif

    #ifdef DEBUG_SHOW_CLIP
        float clip = max(max(pixel_out.r, pixel_out.g), pixel_out.b) ;
        if (clip  > 1.0)
            pixel_out = 1-pixel_out;//;-clip;
    #endif

    FragColor = vec4(pixel_out, 1.0);
    
}

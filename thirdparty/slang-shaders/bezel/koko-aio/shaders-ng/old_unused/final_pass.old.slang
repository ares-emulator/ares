#version 450

/* This pass:
 * Composes the previous passes
 * Does masks, spot, bezel, vignette, background image (anything else?)
 */

#include "config.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;


layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vOutputCoord;
layout(location = 2) out vec2 spot_offset;
layout(location = 3) out vec2 vFragCoord;
layout(location = 4) out float vIsRotated;


//Curvature:
    layout(location = 5) out vec2 vWarp_vexp;
    layout(location = 6) out vec2 vWarp_arg2;

//Scanlines:
    layout(location = 7) out float vIsInterlaced;
    layout(location = 8) out float vFlickeringScanlines;
    layout(location = 9) out float vScanlinePeriod;
    layout(location = 10) out float vScanlinesAlternate;

//Vignette, spot:
    layout(location = 11) out float vIn_aspect;

layout(location = 12) out float vBEZEL_INNER_ZOOM_adapted;

#include "includes/functions.include.slang"


void main() {
    gl_Position = global.MVP * Position;
    vIsRotated = float(is_rotated());
    vIn_aspect = get_in_aspect();
    

    
    
    //Calculate vTexcoord as fractional or integer scaled?
        if ( !do_integer_scale() )
            vTexCoord = get_scaled_coords_aspect(TexCoord,global.FinalViewportSize, vIn_aspect, bool(vIsRotated)) + vec2(0.0001);
                else
            vTexCoord = integer_scale(TexCoord, vIn_aspect, bool(vIsRotated)) + vec2(0.0001); //<- without the offset, SCANLINE_MIN doesn't work.

                        
        
    //if (DO_GLOBAL_SHZO >0.5)
    //    vTexCoord = zoom(vTexCoord + vec2(-GLOBAL_OFFX, -GLOBAL_OFFY), GLOBAL_ZOOM );
    //..unbranched previous
    vTexCoord = (zoom(vTexCoord + vec2(-GLOBAL_OFFX, -GLOBAL_OFFY), GLOBAL_ZOOM ) * DO_GLOBAL_SHZO) + 
                (vTexCoord * (1-DO_GLOBAL_SHZO) );

    vOutputCoord = TexCoord;

    vFragCoord = vec2( floor(vOutputCoord.x * params.OutputSize.x),
                       floor(vOutputCoord.y * params.OutputSize.y));

    
    vBEZEL_INNER_ZOOM_adapted = get_BEZEL_INNER_ZOOM() * DO_BEZEL;
    //Precalc some Curvature/Warp values:
        float warp_adaption =  1 - vBEZEL_INNER_ZOOM_adapted * 4.0;
        vec2 geom_warp_adapted = vec2(GEOM_WARP_X, GEOM_WARP_Y) * warp_adaption;

        vWarp_vexp = 1.0/ (1 + (vec2(geom_warp_adapted.x, geom_warp_adapted.y) * 0.2)) ;
        vWarp_arg2 = 1.0 - pow(vec2(0.29289321881345247559915563789515), vWarp_vexp );

    //SPOT
        spot_offset = offsets_from_float(S_POSITION+420.0,40);
        spot_offset = spot_offset / 10.0 + vec2(0.0,1.0);
        
    //Help scanline code too:
        if (DO_SCANLINES > 0.5) {
            bool bIs_Interlaced = is_interlaced();
            vIsInterlaced = float(bIs_Interlaced);
            vFlickeringScanlines = float((DO_SCANLINES == 1.0) && scanline_have_to_flicker(bIs_Interlaced)) ;

            //Scanline period:
            vScanlinePeriod = 1.0;
            vScanlinesAlternate = 0.0;
            if (bIs_Interlaced) {
                if (params.FrameCount % 2 == 0.0) vScanlinesAlternate = 1.0;
                if (params.OriginalSize.y > MIN_LINES_INTERLACED )  vScanlinePeriod = 0.5;
            }
            
            //vScanlinePhase = 0.0;
            //vScanlinePhase = vScanlinePeriod * pi - pi/4.0; //<-- needed to match scanlines done in first_pass. //FIXME check what happens for highter res.
        }
}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vOutputCoord;
layout(location = 2) in vec2 spot_offset;
layout(location = 3) in vec2 vFragCoord;
layout(location = 4) in float vIsRotated;
layout(location = 5) in vec2 vWarp_vexp;
layout(location = 6) in vec2 vWarp_arg2;
layout(location = 7) in float vIsInterlaced;
layout(location = 8) in float vFlickeringScanlines;
layout(location = 9) in float vScanlinePeriod;
layout(location = 10) in float vScanlinesAlternate;
layout(location = 11) in float vIn_aspect;
layout(location = 12) in float vBEZEL_INNER_ZOOM_adapted;

layout(location = 0) out vec4 FragColor;


#ifdef STATIC_SUPPORT_BACKDROP
    layout(set = 0, binding = 1) uniform sampler2D backdrop;
        #else
    layout(set = 0, binding = 1) uniform sampler2D first_pass;
#endif
layout(set = 0, binding = 2)  uniform sampler2D bloom_pass_final;
layout(set = 0, binding = 3)  uniform sampler2D reflected_blurred_pass;
layout(set = 0, binding = 4)  uniform sampler2D ambi_temporal_pass;
layout(set = 0, binding = 5)  uniform sampler2D avglum_pass;
layout(set = 0, binding = 6)  uniform sampler2D monitor_body_straight;
layout(set = 0, binding = 7)  uniform sampler2D monitor_body_curved;
layout(set = 0, binding = 8)  uniform sampler2D bg_under;
layout(set = 0, binding = 9)  uniform sampler2D bg_over;
layout(set = 0, binding = 10) uniform sampler2D shift_and_bleed_pass;
layout(set = 0, binding = 11) uniform sampler2D in_glow_pass;
layout(set = 0, binding = 12) uniform sampler2D halo_pass;


#define HALF_PI 1.5707963267949
#define QUARTER_PI 0.785398163397448

#include "includes/functions.include.slang"


vec2 vOutputCoord_adapted;
vec2 get_vOutputCoord_adapted() {
    return vOutputCoord_adapted;
}

vec3 pixel_vmask(vec3 source, float white_reference, float over_white) {
    // Simple RGB mask emulation with or without horizontal gap
    float ggg = 1.0 - RGB_MASK_STRENGTH;
    float ooo = max( VMASK_USE_DUMB + 1 - RGB_MASK_STRENGTH, 0.0); //<- make a dumb mask?

    // RGB mask:     R     G     B
    vec3 m1 =  vec3( 1.0 , ooo , ooo );  //col 1
    vec3 m2 =  vec3( ooo , 1.0 , ooo );  //col 2
    vec3 m3 =  vec3( ooo , ooo , 1.0 );  //col 3
    vec3 gap = vec3( ggg );

    vec3 pixel_out;
    vec3 vmasked;
    vec4 vec_mod=(vec4(3,1,2,3) + vec4(VMASK_GAP,0.0,0.0,0.0))* VMASK_DARKLINE_SCALE ;

    float fcol = vOutputCoord.x * params.OutputSize.x;
    float mod_compare = mod(fcol, vec_mod.x);
    
    if (mod_compare < vec_mod.y)
        vmasked = m1;
    else if (mod_compare < vec_mod.z)
        vmasked = m2;
    else if (mod_compare <  vec_mod.w)
        vmasked = m3;
    else vmasked = gap;
    
    vmasked *= source;
    
    //Unbranched previous, worse performance.
        /*
        bool bm1 = (mod_compare < vec_mod.y);
        bool bm2 = (mod_compare < vec_mod.z) && !bm1;
        bool bm3 = (mod_compare < vec_mod.w) && !bm1 && !bm2;
        bool bgap = !bm3 && !bm2 && !bm1;
        vmasked = m1*float(bm1) + m2*float(bm2) + m3*float(bm3) + gap*float(bgap);
        vmasked = source * vmasked;
        */
          
    if (over_white == 1.0) pixel_out = vmasked;
        else {
            white_reference-= over_white;
            white_reference=  clamp(white_reference,0.0,1.0);
            pixel_out=  mix(vmasked,source,white_reference);
        }
   return pixel_out;

}


vec3 pixel_vmask_gm(vec3 source, float white_reference, float over_white) {
    // Simple RGB mask emulation with or without horizontal gap
    float ggg = 1.0 - RGB_MASK_STRENGTH;
    float ooo = max( VMASK_USE_DUMB + 1 - RGB_MASK_STRENGTH, 0.0); //<- make a dumb mask?
    
    // RGB mask:     R     G     B
    vec3 m1 =  vec3( ooo , 1.0 , ooo );    //col 1
    vec3 m2 =  vec3( 1.0 , ooo , 1.0 );    //col 2
    vec3 gap = vec3( ggg );

    float fcol = vOutputCoord.s * params.OutputSize.x;
    vec3 pixel_out;
    vec3 vmasked;
    vec3 vec_mod=(vec3(2,1,2) + vec3(VMASK_GAP,0.0,0.0))* VMASK_DARKLINE_SCALE ;

    float mod_compare = mod(fcol, vec_mod.x);
    
   /* 
    if (mod_compare < vec_mod.y)
        vmasked = m1 * source;
    else if (mod_compare < vec_mod.z)
        vmasked = m2 * source;
    else
        vmasked = gap * source;
    */
   
    //Unbranched previous, same performance.
        bool bm1 = (mod_compare < vec_mod.y);
        bool bm2 = (mod_compare < vec_mod.z) && !bm1;
        bool bgap = !bm2 && !bm1;
        vmasked = m1*float(bm1) + m2*float(bm2) + gap*float(bgap);
        vmasked = source * vmasked;
    
    if (over_white == 1.0) pixel_out = vmasked;
        else {
            white_reference-= over_white;
            white_reference=  clamp(white_reference,0.0,1.0);
            pixel_out=  mix(vmasked,source,white_reference);
        }

   return pixel_out;
}


/*vec3 pixel_vmask_and_gm(vec3 source, float white_reference, float over_white) {
    // Simple RGB mask emulation with or without horizontal gap
    float ooo = 1.0 - RGB_MASK_STRENGTH;

    // RGB mask:     R     G     B
    vec3 m1_rgb =  vec3( 1.0 , ooo , ooo );  //col 1
    vec3 m2_rgb =  vec3( ooo , 1.0 , ooo );  //col 2
    vec3 m3_rgb =  vec3( ooo , ooo , 1.0 );  //col 3
    vec3 gap = vec3( ooo );

    vec3 m1_gm = vec3( ooo , 1.0 , ooo );    //col 1
    vec3 m2_gm = vec3( 1.0 , ooo , 1.0 );    //col 2

    vec3 m1 = (m1_gm * VMASK_USE_GM) + (m1_rgb * (1-VMASK_USE_GM));
    vec3 m2 = (m2_gm * VMASK_USE_GM) + (m2_rgb * (1-VMASK_USE_GM));
    vec3 m3 = m3_rgb;

    vec3 pixel_out;
    vec3 vmasked;
    vec4 vec_mod_rgb = vec4(3,1,2,3);
    vec4 vec_mod_gm =  vec4(2,1,2,0.0);
    vec4 vec_mod = (vec_mod_gm * VMASK_USE_GM) + (vec_mod_rgb* (1-VMASK_USE_GM));
    vec_mod=(vec_mod + vec4(VMASK_GAP,0.0,0.0,0.0))* VMASK_DARKLINE_SCALE ;


    float fcol = vOutputCoord.s * params.OutputSize.x;
    if (mod(fcol, vec_mod.x) < vec_mod.y)
        vmasked = m1 * source;
    else if (mod(fcol, vec_mod.x) < vec_mod.z)
        vmasked = m2 * source;
    else if (    mod(fcol, vec_mod.x) <  vec_mod.w && VMASK_USE_GM < 1.0  )
        vmasked = m3 * source;
    else vmasked = gap * source;

    if (over_white == 1.0) pixel_out = vmasked;
        else {
            white_reference-= over_white;
            white_reference=  clamp(white_reference,0.0,1.0);
            pixel_out=  mix(vmasked,source,white_reference);
        }
   return pixel_out;

}*/


vec3 pixel_darklines(vec3 source,float darkline_every, float darkline_trans,
                     float do_offset, float white_reference,float over_white, vec2 coords) {

    float period_x = (3.0 - VMASK_USE_GM + VMASK_GAP) * VMASK_DARKLINE_SCALE;
    float period_y = darkline_every * VMASK_DARKLINE_SCALE;

    vec2 FragCoord = vFragCoord;

    if (over_white != 1.0) {
        //less effect on bright colors.
        darkline_trans+=(white_reference-over_white);
        darkline_trans=clamp(darkline_trans,0.0,1.0);
    }

    if (do_offset > 0.0) {
        float column = int(floor(mod(FragCoord.x, period_x*2.0)));
        if (column < period_x) FragCoord.y+=period_y/2.0;
    }

    float row = int(floor(mod(FragCoord.y, period_y)));

    if (row < VMASK_DARKLINE_SCALE)
        return source * darkline_trans;
            else
        return source;
}


float scanline_shape(vec2 coords, bool do_slotmask, float lum) {
    /* Produces a B/W scanline shape to be multiplicated by the source
    * pixel later on to emulate scanline effect.
    * scanlines can be drawn on odd and even fields alternating on odd and
    * even frames to emulate that aspect of interlacing.
    */

    float period = vScanlinePeriod;
    bool is_interlaced = bool(vIsInterlaced);
    bool alternate = bool(vScanlinesAlternate);
    bool do_flicker = bool(vFlickeringScanlines);

    //...to scale scanline tickness:
    //float lum_scaled = scale_to_range(lum, SCANLINE_MIN, SCANLINE_MAX);
    float lum_scaled = lum * SCANLINE_MAX;
          lum_scaled = clamp( lum_scaled, SCANLINE_MIN, SCANLINE_MAX );


    float angle_base = coords.y * pi * params.OriginalSize.y * period;// + vScanlinePhase;

    float lines = 1.0;

    if (!( is_interlaced && SCANLINE_DISABLE_ON_INTERLACE == 1.0 )) {

        /*
            if (alternate)
                lines = -sin(angle_base + HALF_PI);
                    else
                lines = sin(angle_base);
        */
        //Unbranched the previous if/then/else:
        lines = (2*vScanlinesAlternate -1) * sin(angle_base + (vScanlinesAlternate * HALF_PI)) ;

        lines = (lines*lines);
    }

    //Draw the slotmask right here if the height is the same as the scanline.
    if (do_slotmask) {
        if (do_flicker) {
            //Fallback to old darklines code to avoid glitches/artifacts.
            lines = lines * pixel_darklines(vec3(1.0),DARKLINES_PERIOD,1-SCANLINE_SM_STRENGTH,1.0, 0.0 ,1.0, coords).r;

        } else {
            float Darkline_part_w = (3.0 - VMASK_USE_GM + VMASK_GAP) * VMASK_DARKLINE_SCALE;
            float Darkline_part_w_x2 = Darkline_part_w*2;
            bool odd_column = mod(vFragCoord.x + Darkline_part_w  , Darkline_part_w_x2) < Darkline_part_w;

            if (SCANLINE_SM_TYPE == 1.0) {
                // Slotmask is out oh phase every "odd" triple and mixed with
                // the straight scanline.
                // slotmask strength and stagger are configurable.
                // Acceptable values are with stagger around 79 (pi/4)
                if (odd_column) {
                    float phase = pi + SCANLINE_SM_VOFFSET*0.01 * lum_scaled;
                    float angle_slotmask = angle_base + phase;
                    float slotmask = sin(angle_slotmask);
                    slotmask = (slotmask*slotmask);
                    lines = mix(lines, slotmask, SCANLINE_SM_STRENGTH);
                }
            }

            else if (SCANLINE_SM_TYPE == 2.0) {
                float phase;
                //Slotmask is always out of phase by QUARTER_PI
                //Every "odd triple" column, it is also out of phase of HALF_PI
                //Slotmask is multiplied by original scanline and thinner.
                //Only strength parameter is taken into account.
               if (odd_column)
                    phase = period * pi + QUARTER_PI * lum_scaled;
                        else
                    phase = period * HALF_PI + QUARTER_PI;

                float angle_slotmask = angle_base + phase;
                float slotmask = sin(angle_slotmask);
                slotmask = slotmask*slotmask; slotmask = slotmask*slotmask; // eleva alla 4 per renderla sottile. valori maggiori producono moiree.
                slotmask = mix(1.0, slotmask, SCANLINE_SM_STRENGTH); // riduci la forza della slotmask
                lines = lines * slotmask;
            }

            else if (SCANLINE_SM_TYPE == 3.0) {
                // Thinner slotmask screen, no scanlines.
                // No parameter taken into account.
                if (odd_column) {
                    float phase = pi + HALF_PI;
                    float angle_slotmask = angle_base + phase;
                    float slotmask = sin(angle_slotmask);
                    slotmask = (slotmask*slotmask); slotmask = (slotmask*slotmask);
                    lines = slotmask;
                }
            }
        }
    }


    //Thickness (min):

        //lines = pow(lines, 1/(lum_scaled+eps) );
        //lines = pow(lines, 1/(sqrt(lum_scaled) ) );
        //lines = pow(lines, 1/(lum_scaled*lum_scaled+eps) );

        //lines = pow( lines , 1/(lum_scaled ) ) ;

        //0.0125 is an anti-moiree measure.

        //lines = pow(   max(lines, 0.05),   1/lum_scaled) ;

        //lines = pow(lines, 1/(sqrt(lum_scaled) ) );

        //lines = pow(clamp(lines + 0.05, 0.0,1.0) , 1/sqrt(lum_scaled));
        //float extra_power_1 = 3.0;

        

        
        float extra_power_1 = max(SCANLINE_MAX/3.0, 1.0);

        lines = pow(clamp(lines, SCANLINE_DARK * 2.0, 1.0) , 1/(sqrt(lum_scaled*lum_scaled*extra_power_1) ) );

        //lines = pow(clamp(lines + 0.05, 0.0,1.0) , 1/(sqrt(lum_scaled*lum_scaled*extra_power_1) ) ) * extra_power_1/2.0;


        //lines = pow(clamp(lines + 0.05, 0.0,1.0) , 1/(lum_scaled*lum_scaled));

        //Thickness (max):

            /*lines = pow(abs(lines), 1/(10*lum_scaled*lum_scaled));
            float clamp_start=0.5;
            if (lum_scaled>clamp_start) lines = clamp(lines,(lum_scaled-clamp_start)/(1-clamp_start) ,1.0); //FIXME use step?*/

    //min-max, ma difficile da gestire con la slotmask.
    //    float extra_power = 2.0;
    //    float m = mod(angle_base + (vScanlinesAlternate * HALF_PI), pi);
    //    lines = exp( -pow((m-(HALF_PI)),2) / ( lum*extra_power ) ) * extra_power;


    lines = lines * (1-SCANLINE_DARK ) + SCANLINE_DARK ;
    return lines;
}

float get_clamped_white_reference(vec3 pixel_in){
    float white_reference = max(max(pixel_in.r,pixel_in.g),pixel_in.b);
    white_reference = min(white_reference,1.0); //Clamp here for both vmask and darklines.
    return white_reference;
}

vec4 pixel_main_pass(vec2 coords, bool bBorder_needed) {
    //Exit if we don't need a border (ambient light/background picture)
    //And we are outside the the border (unsure if this double check is good for performance) //FIXME
    if (bBorder_needed &&
       (vTexCoord.x < 0.0 || vTexCoord.x > 1.0 || vTexCoord.y < 0.0 || vTexCoord.y > 1.0))
       return vec4(0.0);

    vec3 pixel_in;
    vec3 pixel_glowed;


    //Get the first available pixel_in:
    //vec2 off=vec2(0.0,+ global.first_passSize.w*0.5); needed
    //To be added to the following coords and to halo coords (FIXME: WHY?)
    //if drawing scanlines on first pass too)
    if  (DO_IN_GLOW == 1.0) {
            pixel_glowed = texture(in_glow_pass,coords).rgb;
            pixel_in = pixel_glowed;
    } else {
        pixel_in = texture(shift_and_bleed_pass,coords).rgb ;
    }


    vec3 pixel_out = pixel_in;
    float white_reference;
    if (VMASK_OVERWHITE+DRKLN_OVERWHITE  < 2.0 ) {
        white_reference = get_clamped_white_reference(pixel_in);
    }

    //Mask and darklines:
    if (DO_VMASK_AND_DARKLINES == 1.0 ) {
        vec3 pixel_in_compensated = pixel_in;

        pixel_in_compensated = pixel_push_luminance(pixel_in, RGB_MASK_STRENGTH * MASK_COMPENSATION);

        if (RGB_MASK_STRENGTH > 0.0) {
            //Use RGB pattern or exploit RGB layout with green and magenta?
            if (VMASK_USE_GM < 1.0)
                pixel_out = pixel_vmask(pixel_in_compensated, white_reference, VMASK_OVERWHITE);
            else
                pixel_out = pixel_vmask_gm(pixel_in_compensated, white_reference, VMASK_OVERWHITE);
            //Tried to unbranch to no avail (worse performances)
            //pixel_out = pixel_vmask_and_gm(pixel_in_compensated, white_reference, VMASK_OVERWHITE);
        }

        pixel_out = pixel_push_luminance(pixel_out, DARKLINES_STRENGTH * (1.0/DARKLINES_PERIOD) * MASK_COMPENSATION);

        //Screen lines (slotmask aperture grille)
        if (   DARKLINES_STRENGTH > 0.0 ) {
            float MYDARKLINES_TRANSPARENCY = 1.0 - DARKLINES_STRENGTH;
            pixel_out = pixel_darklines(pixel_out, DARKLINES_PERIOD, MYDARKLINES_TRANSPARENCY, DARKLINES_VOFFSET, white_reference, DRKLN_OVERWHITE, coords);
        }

    }

    float lum = clamp(max(max(pixel_out.r,pixel_out.g),pixel_out.b), 0.0,1.0);

    //Halo
    vec3 pixel_haloed;
    if (DO_HALO == 1.0 ) {
        pixel_haloed = texture(halo_pass,coords).rgb;
        pixel_out += pixel_haloed * (1 - HALO_VS_SCAN);
    }

    /* Non flickering scanlines, Don't do them if
     * - User doesn't want scanlines
     * - The screen is interlaced and the user doesn't want scanlines on it.
     */
    if  ( DO_SCANLINES == 1.0 ) {
        /* pixel_bleed is the color that will bleed over scanline gap.
         * It is selected by the first available one depending on the shader
         * features enabled by the user */
        vec3 pixel_bleed;
        /*if (DO_HALO == 1.0)
            pixel_bleed = pixel_haloed;
        else */if (DO_IN_GLOW == 1.0)
            pixel_bleed = pixel_glowed;
        else
            pixel_bleed = pixel_in;

        // Optionally apply a gamma correction to the scanline shape.
        if (SCANLINE_COMPENSATION > 0.0) {
            float mypow = mix(1.0, (0.33 * SCANLINE_DARK + 0.67), SCANLINE_COMPENSATION);
            pixel_out = pow(pixel_out,vec3(mypow));
        }

        //Obtain the scanlines screen by multiplying the scanline shape by the pixel color.
        //and emulate the bleeding of the color over the dark part of the scanlined screen.
        //float scanline_shape = scanline_shape(coords, SCANLINE_SM_TYPE > 0.0 );
            float lum = max(max(pixel_out.r, pixel_out.g), pixel_out.b);
            float scanline_shape = scanline_shape(coords, SCANLINE_SM_TYPE > 0.0, lum );

            pixel_out = pixel_out * ( scanline_shape + ( pixel_bleed * SCANLINES_BLEEDING * ( 1 - scanline_shape ) ));
    }


     if (DO_HALO == 1.0 ) {
        pixel_out += pixel_haloed * HALO_VS_SCAN;
     }

    // Apply color correction (in this pass it is just the final output gamma)
    if (DO_CCORRECTION == 1.0) pixel_out = pow(max(pixel_out, vec3(0.0)),vec3(GAMMA_OUT));

    //Out
    return vec4(pixel_out,1.0) ; //* border(border_coords);
}

float global_noise;

#ifdef STATIC_SUPPORT_BACKDROP
    vec3 pixel_backdrop_image() {
        vec2 backdrop_offset=vec2(BACKDROP_OFFX,BACKDROP_OFFY);

        vec2 backdrop_tex_size = textureSize(backdrop, 0);
        float backdrop_lod = log2(backdrop_tex_size.y / global.FinalViewportSize.y);

        vec2 backdrop_coords =  get_scaled_coords_aspect(
                                vOutputCoord+backdrop_offset,
                                global.FinalViewportSize,
                                backdrop_tex_size.x/backdrop_tex_size.y,
                                bool(vIsRotated));

        backdrop_coords=zoom(backdrop_coords, BACKDROP_ZOOM);
        return textureLod(backdrop, backdrop_coords, backdrop_lod).rgb;
    }
#endif

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
            bool bOutside = (co.x < 0.0 || co.x > 1.0 || co.y < 0.0 || co.y > 1.0 ) ;
            if (bOutside) return vec4(0.0,0.0,0.0,1.0);
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


vec4 pixel_background_image(bool over){
    //return the aspect corrected background image:
    vec2 bg_image_offset=vec2(BG_IMAGE_OFFX,BG_IMAGE_OFFY);
    if (over) {
        vec2 tex_size = textureSize(bg_over, 0); // * BG_ZOOM;
        float bg_over_lod = log2(tex_size.y / global.FinalViewportSize.y);

        if (BG_IMAGE_ROTATION > 0.0  || ( BG_IMAGE_ROTATION < 0.0 && bool(vIsRotated) )  ) tex_size.xy = tex_size.yx;

        vec2 back_coords = get_scaled_coords_aspect(vOutputCoord + bg_image_offset, global.FinalViewportSize, tex_size.x/tex_size.y, bool(vIsRotated));

        if (BG_IMAGE_ROTATION < 0.0 && bool(vIsRotated) ) {
            //handle automatic rotation of bg image for rotated games
            back_coords.xy = back_coords.yx;
            back_coords.y = 1 - back_coords.y;
        } else if (BG_IMAGE_ROTATION > 0.0) {
            //rotate as user prefs
            back_coords.xy = back_coords.yx;
            if (BG_IMAGE_ROTATION == 1.0) back_coords.y = 1 - back_coords.y;
            if (BG_IMAGE_ROTATION == 2.0) back_coords.x = 1 - back_coords.x;
        }
        back_coords=zoom(back_coords, BG_IMAGE_ZOOM);
        vec4 pixel_bgover = textureLod_wrap(bg_over, back_coords, bg_over_lod, BG_IMAGE_WRAP_MODE);
        return pixel_bgover;
        
        
        
    }
    //under
        vec2 tex_size = textureSize(bg_under, 0); // * BG_ZOOM;
        if (BG_IMAGE_ROTATION > 0.0) tex_size.xy = tex_size.yx;
        float bg_under_lod = log2(tex_size.y / global.FinalViewportSize.y);
        vec2 back_coords = get_scaled_coords_aspect(vOutputCoord + bg_image_offset,global.FinalViewportSize, tex_size.x/tex_size.y, bool(vIsRotated));
        if (BG_IMAGE_ROTATION > 0.0) {
            back_coords.xy = vec2(back_coords.y, back_coords.x);
            if (BG_IMAGE_ROTATION == 1.0) back_coords.y = 1 - back_coords.y;
            if (BG_IMAGE_ROTATION == 2.0) back_coords.x = 1 - back_coords.x;
        }
        back_coords=zoom(back_coords, BG_IMAGE_ZOOM);
        return textureLod_wrap(bg_under,back_coords,bg_under_lod, BG_IMAGE_WRAP_MODE);
        //return textureLod(bg_under,back_coords,bg_under_lod);
}

vec3 pixel_border(bool image_over) {
    /* Returns the surrounding of the tube/bezel:
     * The led lights alone
     * the led light "under" a background image (alpha driven)
     * black if we need to overlay a game backdrop
     */
    #ifdef STATIC_SUPPORT_BACKDROP
        return vec3(0.0);
    #endif
    vec3 pixel_out = vec3(1.0);
    if (DO_AMBILIGHT == 1.0)
        pixel_out =  texture(ambi_temporal_pass, vOutputCoord).rgb + global_noise;
    if (DO_BG_IMAGE == 1.0 && !image_over) {
        vec4 bg_image = pixel_background_image(false);
        // Mix or add ambilight to the background image?
            //...with ternary operator:
            pixel_out = (AMBI_BG_IMAGE_BLEND_MODE == 0.0) ? 
                mix(pixel_out,bg_image.rgb,bg_image.a) 
                    : 
                bg_image.rgb + (pixel_out * (1 - bg_image.a));
            //...with if/then:
            /*if (AMBI_BG_IMAGE_BLEND_MODE == 0.0)
                pixel_out = mix(pixel_out,bg_image.rgb,bg_image.a);
                    else
                pixel_out = bg_image.rgb + (pixel_out * (1 - bg_image.a));
            */
            //..unbranched versions:
            /*  ...mix is the slower.
                vec3 pixel_out_alpha_blend_mix = mix(pixel_out,bg_image.rgb,bg_image.a);
                vec3 pixel_out_alpha_blend_add = bg_image.rgb + (pixel_out * (1 - bg_image.a));
                pixel_out = (pixel_out_alpha_blend_add *  AMBI_BG_IMAGE_BLEND_MODE) + 
                            (pixel_out_alpha_blend_mix *  (1-AMBI_BG_IMAGE_BLEND_MODE));
            */

        
    }
    return pixel_out;
}


vec3 pixel_alternate(vec3 source, float whiteness) {
    // Emulate the low crt persistance by only drawing odd/even lines
    // on odd/even frames
    float line = vTexCoord.y * params.OutputSize.y;
    vec3 pixel_out = source;
    float l_period_half = ALT_BLANK_PERIOD / 2;
    if  (mod(float(params.FrameCount),2.0  ) == 1) {
        if  (mod(line,ALT_BLANK_PERIOD) > l_period_half) pixel_out=mix(source,vec3(0), whiteness) ;
    } else {
        if  (mod(line,ALT_BLANK_PERIOD) <= l_period_half) pixel_out=mix(source,vec3(0), whiteness) ;
    }
    return pixel_out;
}


vec3 bezel_color(float lum)  {
    //Colorize bezel frame
    vec3 col = vec3(BEZEL_R,BEZEL_G,BEZEL_B) + lum;
    float con_max =  0.5 * BEZEL_CON + 0.5;
    col = scale_to_range_vec3(col, -con_max+1, con_max);
    return clamp(col,0.0,1.0);
}


float fuzzyrect(vec2 uv, vec2 size, float radius, float blur) {
    vec2 hSize = size / 2.0 - radius;
    float d = length(max(abs(uv - vec2(0.5)),hSize)-hSize);
    return smoothstep(-radius-blur, -radius+blur, -d);
}

float create_ambi_colorize_shade(vec2 co) {
    float blur = AMBI_OVER_BEZEL_SIZE;
    vec2 size = vec2(1.0, 2 - AMBI_OVER_BEZEL_AR_CORRECTION)-blur;
    float radius = 0.0;

    return  1 - min ( fuzzyrect(co, size,  radius,  blur) * 2, 1.0);
}
    

vec3 compose_bezel_over(vec3 full_viewport, vec2 coords_for_bezel, vec2 coords_for_mirror) {

    //Exit if we are outside bezel coordinates
        if (coords_for_bezel.x < 0  || coords_for_bezel.y < 0 || coords_for_bezel.x > 1.0 || coords_for_bezel.y > 1.0)
            return full_viewport;

    //Sample main bezel texture:
        vec4 bezel;
        #ifdef STATIC_BEZEL_USE_MIPMAP
            vec2 bezel_lut_size = textureSize(monitor_body_curved,0); //no need to branch here if we assume straight and curved textures are the same size. //FIXME?
            float bezel_frame_lod = log2(bezel_lut_size.y * (BEZEL_FRAME_ZOOM+1.0) /global.FinalViewportSize.y);
            if (BEZEL_USE_STRAIGHT < 0.5)
                bezel = textureLod(monitor_body_curved,coords_for_bezel,bezel_frame_lod);
                    else
                bezel = textureLod(monitor_body_straight,coords_for_bezel,bezel_frame_lod);
        #else
            if (BEZEL_USE_STRAIGHT < 0.5)
                bezel = texture(monitor_body_curved,coords_for_bezel);
                    else
                bezel = texture(monitor_body_straight,coords_for_bezel);
        #endif
    

    //Exit if the bezel is completely transparent (the tube)
        if (bezel.a == 0.0) return full_viewport;
        
    //Colorize the bezel (bezel.r expresses the luminance)
        vec3 bezel_colored = bezel_color(bezel.r);
    
    //Calculate the bezel roughness to apply to the reflecting area. (bezel.g expresses the specularity)
        float lut_specular = bezel.g * BEZEL_SPCL_STRENGTH;
        float roughness = random( min(global.FinalViewportSize.z,global.FinalViewportSize.w) * BEZEL_ROUGHNESS, vTexCoord );
        roughness *=  (1 - min(lut_specular * 10, 1.0)); // <-roughness over specular area looks bad.
        
    //Reflections:
        float reflection_modifier = bezel.b * BEZEL_REFL_STRENGTH; //bezel.b expresses how much the area is reflective
        
        //Sample the reflection pass with small offset to simulate the roughness
            vec3 pixel_mirrored = texture(reflected_blurred_pass, coords_for_mirror + roughness).rgb; 
            float fcorners_shade = corners_shade(coords_for_bezel, 1.0);
            pixel_mirrored*=1-(fcorners_shade);
            
        //Push it over the specular areas and apply the reflection modifier
            pixel_mirrored = pixel_mirrored * (1 + lut_specular) * reflection_modifier;

        //Apply reflections
            vec3 bezel_out = bezel_colored + pixel_mirrored;
            
    
    //Mix ambient light over bezel? (branching this is worse)
        vec3 ambitemporalpass = texture(ambi_temporal_pass, vOutputCoord ).rgb;
        ambitemporalpass = max(ambitemporalpass, 0.0); //<-- antiglitch. Needed to avoid undefined variable on vulkan.
        vec3 pixel_ambilight = DO_AMBILIGHT * ( ambitemporalpass + global_noise);
            
        //Create a shaded box to apply ambilight on external borders only
            float ambi_colorize_shade = create_ambi_colorize_shade(coords_for_bezel);
            bezel_out = bezel_out + (AMBI_OVER_BEZEL * pixel_ambilight * bezel.a * ambi_colorize_shade * (1-reflection_modifier));

    //Diffuse the light over specular areas, we use a mipmap with low precision.
        if (lut_specular > 0.0) {
            vec4 pixel_avglum = texture(avglum_pass, coords_for_mirror);
            bezel_out = bezel_out + (pixel_avglum.rgb * lut_specular) ;
        }
    
    return mix(full_viewport, bezel_out, bezel.a);
}

vec4 pixel_inner_frame(vec2 coords, bool bBorder_needed) {
//This takes care of drawing the main content.

    vec4 pixel_in = pixel_main_pass(coords, bBorder_needed);
    vec3 pixel_out = pixel_in.rgb;
    vec3 bloomed;

    //Mix bloom texture
    if (DO_BLOOM == 1.0 ) {
        bloomed=texture(bloom_pass_final, coords).rgb ;
        
        /*if (BLOOM_BYPASS > 0.5)
            pixel_out = bloomed;
        else
            pixel_out += bloomed;
        */
        //unbranched version, maybe a bit faster.
        pixel_out = bloomed + float(BLOOM_BYPASS < 0.5) * pixel_out;
        //pixel_out = bloomed + (pixel_out * (1 - min(BLOOM_BYPASS, 1.0) ));
    }

    //Black frame insertion, made static, enable it in config.inc.
    #ifdef ALLOW_ALT_BLANK
        if (DO_ALT_BLANK == 1.0 ) {
            pixel_out = pixel_alternate(pixel_out.rgb, ALT_BLANK_STRENGTH );
        }
    #endif

    //vignette and spot
        if (DO_VIGNETTE + DO_SPOT >0.0) {
            //float in_aspect = get_in_aspect();
            //pixel_out = clamp(pixel_out, 0.0,1.0); //<--unsure
            if (DO_VIGNETTE == 1.0) {
                float dist = length(vec2((coords.x-0.5)*vIn_aspect,coords.y-0.5));
                float vignette = smoothstep(V_SIZE,0.0,dist)*V_POWER;
                //float vignette = vV_POWER * (vV_SIZE + 16.0* coords.x*coords.y*(1.0-coords.x)*(1.0-coords.y));
                //vignette = min(vignette,99999.0); //<--- big wtf here, why is this needed?
                ////...see: https://github.com/kokoko3k/koko-aio-slang/issues/3
                pixel_out = pixel_out * vignette;
            }

            if (DO_SPOT == 1.0) {
                float dist = length(vec2((coords.x-0.5)*vIn_aspect,coords.y-0.5)+spot_offset);
                float spot = smoothstep(S_SIZE,0.0,dist)*S_POWER;
                //vec2 scoords = clamp(coords + spot_offset, 0.0,1.0);
                //float spot = vS_POWER * (vS_SIZE + 16.0*scoords.x*scoords.y*(1.0-scoords.x)*(1.0-scoords.y));
                pixel_out = pixel_out + spot;
            }
            //pixel_out += global_noise;
        }

    //smooth border (edge)
        float border_inner_frame = 1.0;
        if (DO_BEZEL==1.0 || DO_CURVATURE == 1.0)
            border_inner_frame = border(coords);

        float pixel_out_alpha = pixel_in.a * border_inner_frame;

    //Out
        return vec4(pixel_out,pixel_out_alpha) * border_inner_frame;
}


vec2 get_scaled_offset(vec2 foffset, vec2 co1, vec2 co2) {
    // dato un offset e due coordinate, ritorna l'offset 
    // sommato alla differenza delle coordinate.
    
    //Branched and unbranched version, they perform almost the same.
    
    vec2 off_left_up = co1 - co2;
    vec2 off_right_bottom = co2 - co1;
    vec2 bool_is_side_right_bottom = step(0.5, co2);
    vec2 bool_is_side_left_up = 1 - bool_is_side_right_bottom;
    return foffset - (off_left_up * bool_is_side_left_up + 
                            off_right_bottom * bool_is_side_right_bottom);

    /*
    vec2 offset_out;
    if (vTexCoord.x > 0.5)
        offset_out.x = foffset.x - (co1.x - co2.x);
            else
        offset_out.x = foffset.x - (co2.x - co1.x);


    if (vTexCoord.y > 0.5)
        offset_out.y = foffset.y - (co1.y - co2.y);
            else
        offset_out.y = foffset.y - (co2.y - co1.y);

    return offset_out;
    */
}

vec4 main_wrap(vec2 coords, vec2 outer_frame_offset){
/* Handle inner border and outer border, not bezel */

    //Calc a random noise if we need debanding
        if (DO_VIGNETTE + DO_SPOT + DO_AMBILIGHT >0.0) global_noise = random(NOISEPOWER, vTexCoord * params.FrameCount);

    //Calc outside coords:
        bool bIs_outer_frame = vTexCoord.x < 0.0 + outer_frame_offset.x || vTexCoord.x > 1.0 - outer_frame_offset.x ||
                               vTexCoord.y < 0.0 + outer_frame_offset.y || vTexCoord.y > 1.0 - outer_frame_offset.y ;
    
    //If we don't need a border, we may still need to blank the outside area when doing integer scaling:
        if (!border_needed()) {
            return pixel_inner_frame(coords, false) * float(!bIs_outer_frame);
        }


    /* From now on, we need a real border, so start handle outer frame cases */


    //if we want ambilights or back/foreground images in the outer frame, draw it and return
        if (bIs_outer_frame)
            return vec4(pixel_border(BG_IMAGE_OVER == 1.0),1.0);

    //Handle particular (?) inner frame cases
        vec4 vPixel_inner_frame = pixel_inner_frame(coords, true);
        

        bool draw_border = true;
        //Not painting ambilights here is the best way to cheaply draw a border around the frame
        if (DO_BEZEL == 1.0) {
            vec2 vTexCoord_zoomout = zoomout_coords(vTexCoord, (-BEZEL_FRAME_ZOOM + 0.1) , 1.0);
            //Not using curved coords here because crazy things would happen with vertical games
            draw_border = (vTexCoord_zoomout.x < 0.0 || vTexCoord_zoomout.x > 1.0 ||
                            vTexCoord_zoomout.y < 0.0 || vTexCoord_zoomout.y > 1.0) ;
        }
        //mix ambilight in the inner frame too, as curvature could shrink into the inner frame.
            if (draw_border)
                return vec4(mix(pixel_border(BG_IMAGE_OVER == 1.0).rgb,vPixel_inner_frame.rgb,min(vPixel_inner_frame.a*1.5,1.0)),vPixel_inner_frame.a);
        

    //Last case, inner border without ambilight:
        return vPixel_inner_frame;
}



vec2 get_scaled_coords_for_bezel() {
    //This function is here because compiler gets mad if i calc coords_for_bezel
    //outside the main branch "if DO_BEZEL then compose_bezel_over"
    //performances falls down for no apparent reason.
    //But still, i need to access it twice in the code.
    //So this is a function that ensures me that i always calc it the same way.
    
    vec2 co = zoomout_coords(vTexCoord,-BEZEL_FRAME_ZOOM,1.0);
    return co;
    //return tilt(co, vIsRotated, TILT_X, TILT_Y);
}






void main() {

    vec2 coords = vTexCoord;

    vOutputCoord_adapted = tilt(vOutputCoord, vIsRotated, TILT_X, TILT_Y);
    coords = tilt(coords, vIsRotated, TILT_X, TILT_Y);
    
    
    //Curvature
    if (DO_CURVATURE == 1.0  &&  ((GEOM_WARP_X > 0.0) || (GEOM_WARP_Y > 0.0))) {
            coords = Warp_fast(coords, vWarp_vexp, vWarp_arg2);

    }
    vec2 coords_for_mirror = coords;

    //Scale coords according to bezel settings?
    vec2 outer_frame_offset = vec2(0.0);
    
    if (DO_BEZEL == 1.0) {
        coords = zoomout_coords(coords, -vBEZEL_INNER_ZOOM_adapted , 1.0);
        coords_for_mirror = zoom(coords, 1/BEZEL_REFL_ZOOMOUT_ROOM);
        vec2 coords_for_bezel1 = get_scaled_coords_for_bezel();
        outer_frame_offset = get_scaled_offset(BEZEL_BORDER_SIZE, coords_for_bezel1, vTexCoord);
    }

    if (DO_GAME_GEOM_OVERRIDE > 0.5)
        coords = content_geom_override(coords, GAME_GEOM_ASPECT, vIn_aspect, GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM);

    //Luminosity dependant zoom
    if (DO_DYNZOOM == 1.0) {
        //float zoomin = 1.0 + (texture(bloom_pass_final, coords   ).a/ DYNZOOM_FACTOR * 80.0); <<-- funny.
        float dynzoom = get_dyn_zoom(avglum_pass);
        coords = zoom(coords, dynzoom);
    }


    //"Just" handle inner and outer frame here:
    vec4 pixel_out = main_wrap(coords, outer_frame_offset);


    #ifdef STATIC_SUPPORT_BACKDROP
        pixel_out += vec4(pixel_backdrop_image(), 1.0);
    #endif


    //Draw bezel
    if (DO_BEZEL == 1.0) {
        vec2 coords_for_bezel2 = get_scaled_coords_for_bezel();
        pixel_out = vec4(compose_bezel_over(pixel_out.rgb, coords_for_bezel2, coords_for_mirror),1.0);
    }

    //Draw an image "Over" the bezel with an hole inside by the alpha channel
    if (DO_BG_IMAGE == 1.0 && BG_IMAGE_OVER==1.0) {
        vec4 bg_image = pixel_background_image(true);
        
        
        pixel_out.rgb = mix(pixel_out.rgb, bg_image.rgb, bg_image.a );
        
        //If one wants to add instead of mixing ambilight, do it now.
        //Note that mixing done just before is necessary to display the
        //background image.
        if (DO_AMBILIGHT > 0.5 && AMBI_BG_IMAGE_BLEND_MODE > 0.5) { 
            vec3 ambitemporalpass = texture(ambi_temporal_pass, vOutputCoord ).rgb;
            vec3 pixel_ambilight = DO_AMBILIGHT * (ambitemporalpass + global_noise);
            float ambi_over_bezel_mask = create_ambi_colorize_shade(get_scaled_coords_for_bezel());
            float bg_image_alpha_adapted = max(bg_image.a - AMBI_BG_IMAGE_FORCE, 0.0);
            pixel_out.rgb = pixel_out.rgb + (pixel_ambilight.rgb * (ambi_over_bezel_mask) * (1- bg_image_alpha_adapted));
        }

    }

    //For debug purposes:
        //quad split screen
        //if ( (vOutputCoord.x < 0.5  && vOutputCoord.y > 0.5) || (vOutputCoord.x > 0.5  && vOutputCoord.y < 0.5)  ) pixel_out = texture(first_pass,vOutputCoord);
        //split screen
        //if (vTexCoord.y < 0.5) pixel_out = texture(first_pass,vTexCoord);
        //pixel_out = mix(pixel_out, texture(avglum_pass,vTexCoord), 0.8);

    //vec4 pixel_mirrored = texture(reflected_blurred_pass,coords_for_mirror);
    //pixel_out = pixel_mirrored;

    //pixel_out = texture(avglum_pass, vTexCoord);



    FragColor = pixel_out;

    //FragColor = vec4(blur_shade(vTexCoord) );

    //FragColor = vec4(random(1.0, vTexCoord * params.FrameCount));
}


/*86bacef42ab09641329518ea0c932e646fbb014f Allow to just draw gaps in vmasks -1fps
f0b203cd284c74dfc8f252117abd79e1300861cb Implement integer scaling(after optimization) -1.5fps
5ba8439ec540f4920b39569c24acb3244c655fd0 Allow to set a maximum integer scale level -1fps
*/

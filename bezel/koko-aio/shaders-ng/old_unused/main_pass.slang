#version 450
#include "config.inc"
#define half_pi 1.5707963267949

// RGB mask:       R     G     B
 #define m1 vec3 ( 1.0 , 0.0 , 0.0 )    //col 1
 #define m2 vec3 ( 0.0 , 1.0 , 0.0 )    //col 2
 #define m3 vec3 ( 0.0 , 0.0 , 1.0  )   //col 3

// RGB mask low dpi (green-magenta, exploiting native rgb subpixels)
 #define m1gm vec3 ( 0.0 , 1.0 , 0.0 )    //col 1
 #define m2gm vec3 ( 1.0 , 0.0 , 1.0 )    //col 2

// Blur sizes must not depend on input resolution
#define scalemod_x (global.in_glow_passSize.x/360.0)
#define scalemod_y (global.in_glow_passSize.y/270.0)

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vOutputCoord;

#include "includes/functions.include.slang"
void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = get_scaled_coords(TexCoord,global.FinalViewportSize, is_rotated());
   vOutputCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vOutputCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 3) uniform sampler2D FXAA_pass;
layout(set = 0, binding = 4) uniform sampler2D in_glow_pass;
layout(set = 0, binding = 5) uniform sampler2D halo_pass;
layout(set = 0, binding = 6) uniform sampler2D first_pass;
layout(set = 0, binding = 7) uniform sampler2D avglum_passFeedback;


#include "includes/functions.include.slang"
vec3 pixel_vmask(vec3 source, vec3 white_reference, float over_white) {
    int col = int( vOutputCoord.x * params.OutputSize.x );
    vec3 pixel_out;
    vec3 vmasked;
    vec4 vec_mod=(vec4(3,1,2,3) + vec4(VMASK_GAP,0.0,0.0,0.0))* VMASK_DARKLINE_SCALE ;
    float mask_lightness = 1.0 - RGB_MASK_STRENGTH;
    if ( mod(col, vec_mod.x) < vec_mod.y)         vmasked = min(m1 + mask_lightness,vec3(1.0)) * source;
        else if (mod(col, vec_mod.x) < vec_mod.z) vmasked = min(m2 + mask_lightness,vec3(1.0)) * source;
        else if (mod(col, vec_mod.x) < vec_mod.w) vmasked = min(m3 + mask_lightness,vec3(1.0)) * source;
    else vmasked = min(vec3(0.0) + mask_lightness,vec3(1.0)) * source;

    if (over_white == 1.0) pixel_out = vmasked;
        else {
            float whiteness=(white_reference.r+white_reference.g+white_reference.b)/3.0;
            whiteness-= over_white;
            whiteness=  clamp(whiteness,0.0,1.0);
            pixel_out=  mix(vmasked,source,whiteness);
        }
   return pixel_out;
}

vec3 pixel_vmask_gm(vec3 source, vec3 white_reference, float over_white) {
    int col = int( vOutputCoord.x * params.OutputSize.x );
    vec3 pixel_out;
    vec3 vmasked;
    vec3 vec_mod=(vec3(2,1,2) + vec3(VMASK_GAP,0.0,0.0))* VMASK_DARKLINE_SCALE ;
    
    float mask_lightness = 1.0 - RGB_MASK_STRENGTH;
    if (mod(col, vec_mod.x) < vec_mod.y)          vmasked = min(m1gm + mask_lightness,vec3(1.0)) * source;
        else if (mod(col, vec_mod.x) < vec_mod.z) vmasked = min(m2gm + mask_lightness,vec3(1.0)) * source;
    else vmasked = min(vec3(0.0) + mask_lightness,vec3(1.0)) * source;

    if (over_white == 1.0) pixel_out = vmasked;
        else {
            float whiteness=(white_reference.r+white_reference.g+white_reference.b)/3.0;
            whiteness-= over_white;
            whiteness=  clamp(whiteness,0.0,1.0);
            pixel_out=  mix(vmasked,source,whiteness);
        }
    
   return pixel_out;
}

vec3 pixel_darklines(vec3 source,float darkline_every, float darkline_trans,
                     float do_offset, vec3 white_reference,float over_white) {

    float Darkline_part_w = (3.0 - VMASK_USE_GM + VMASK_GAP) * VMASK_DARKLINE_SCALE;
    float Darkline_part_w_x2 = Darkline_part_w*2;
    //vec3 pixel_out=source;
    float col_2 =  vOutputCoord.x * params.OutputSize.x;
    float line_2 = vOutputCoord.y * params.OutputSize.y;

    darkline_every *= VMASK_DARKLINE_SCALE;

    if (over_white != 1.0) {
        //less effect on bright colors.
        float whiteness=(white_reference.r+white_reference.g+white_reference.b)/3.0;
        darkline_trans+=(whiteness-over_white);
        darkline_trans=clamp(darkline_trans,0.0,1.0);
    }

    if (do_offset == 1.0) {
        if  (int(mod(line_2, darkline_every)) < VMASK_DARKLINE_SCALE) {
            if (int(mod(col_2, Darkline_part_w_x2)) < Darkline_part_w) return source * darkline_trans;
        } else if  (int(mod(line_2+(darkline_every/2), darkline_every)) < VMASK_DARKLINE_SCALE ) {
            // DRAW WITH OFFSET:
            col_2+=Darkline_part_w;
            if ((int(mod(col_2, Darkline_part_w_x2))) < Darkline_part_w)
                return source * darkline_trans;
        }
    } else {

        if ( darkline_every >= 0.0)
            if (int(mod(line_2, darkline_every)) < VMASK_DARKLINE_SCALE) return source * darkline_trans;
    }
    return source;
}

/*
float scanline_shape_static_in_lum(vec2 coords, float in_luminance) {
    // Modulate scanline weight via in_luminance
    float period = (params.OriginalSize.y > MIN_LINES_INTERLACED ) ? 2 : 1 ;
    float angle = coords.y * pi * period * params.OriginalSize.y ;
    float lines = abs(sin(angle));
    lines=clamp(lines, SCANLINE_DARK,1.0);
    lines = (lines*lines);
    float w = (SCANLINES_BLEEDING * 9.0 * in_luminance +1 ); // + SCANLINES_BLEEDING;
    lines = pow(lines,1.0/ w );
    return lines;
}
*/

/*
   float scanline_shape_dumb(vec2 coords, bool do_flicker) {
    float source_dy =  global.first_passSize.w ;
    if ( mod(coords.y, source_dy*2) < source_dy ) {
        return 1.0;
    }
    return 0.0;
}
*/

float scanline_shape(vec2 coords, bool do_flicker) {
    bool alternate = false;
    float period = 1.0;
    
    int slow = 1;
    
    if (do_flicker) {
        if (params.FrameCount/slow % 2 == 0.0) alternate = true;
        if (params.OriginalSize.y > MIN_LINES_INTERLACED )  period = 0.5;
    }

    float angle = coords.y * pi * params.OriginalSize.y * period;

    float lines;
    if (alternate)
        lines = -sin(angle+half_pi); //lines = abs(cos(angle));
             else
        lines = sin(angle);
    lines = (lines*lines);
  
    if (SCANLINE_DARK >= 0.0) {
        lines = lines * (1.0 - SCANLINE_DARK) + SCANLINE_DARK;
    } else {        
        float deepness = -SCANLINE_DARK;
        lines = lines * ((1-SCANLINE_DARK) ) + SCANLINE_DARK ;
    }
    
    return lines;
}

vec4 main_wrap(vec2 co) {
    bool isinterlaced = is_interlaced();
    bool flickering_scanlines = (DO_SCANLINES == 1.0) && scanline_have_to_flicker(isinterlaced) ;


    vec3 pixel_in;
    vec3 pixel_glowed;
    
    //Get the first available pixel_in:
    if  (DO_IN_GLOW == 1.0) {
            pixel_glowed = texture(in_glow_pass,co).rgb;
            pixel_in = pixel_glowed;
        }
            else if (DO_FXAA == 1.0) 
        pixel_in = texture(FXAA_pass,co).rgb ;
            else
        pixel_in = texture(first_pass,co).rgb ;

    vec3 pixel_out = pixel_in;
    vec3 pixel_in_clamped = min(pixel_in,vec3(1.0)); //Clamp here for moth vmask and darklines.
    
    //Mask and darklines:
    if (DO_VMASK_AND_DARKLINES == 1.0 ) {
        if (RGB_MASK_STRENGTH > 0.0) {
            //Use RGB pattern or exploit RGB layout with green and magenta?
            if (VMASK_USE_GM < 1.0) {
                pixel_out = pixel_vmask(pixel_in, pixel_in_clamped, VMASK_OVERWHITE);
            } else {           
                pixel_out = pixel_vmask_gm(pixel_in, pixel_in_clamped, VMASK_OVERWHITE);
            }
        }
        //Screen lines
        if (DARKLINES_STRENGTH > 0.0 ) {
            float MYDARKLINES_TRANSPARENCY = 1.0 - DARKLINES_STRENGTH;
            pixel_out = pixel_darklines(pixel_out,DARKLINES_PERIOD,MYDARKLINES_TRANSPARENCY,DO_DARKLINES_VOFFSET,pixel_in_clamped,DRKLN_OVERWHITE);
        }

    }
    
    //Halo
    vec3 pixel_haloed;
    if (DO_HALO == 1.0 ) {
        pixel_haloed = texture(halo_pass,co).rgb;
        pixel_out += pixel_haloed;
        pixel_out = clamp(pixel_out,0.0,HALO_CLAMP);
    }
    
    // Non flickering scanlines
    if ( DO_SCANLINES == 1.0 )  {
        vec3 pixel_bleed ; 
        if  (! ( isinterlaced  && (SCANLINE_DISABLE_ON_INTERLACE == 1.0))  ) {
            if (DO_IN_GLOW + DO_HALO > 0.0) {
                if (DO_HALO == 1.0) pixel_bleed = pixel_haloed;
                else if (DO_IN_GLOW == 1.0) pixel_bleed = pixel_glowed;
                else pixel_bleed = pixel_in;
            }
        
            float scanline_shape =  scanline_shape(co, flickering_scanlines ) ;
            vec3 pixel_scanlined = scanline_shape * pixel_out;

            pixel_out = pixel_scanlined + (pixel_out * pixel_bleed *  SCANLINES_BLEEDING * (1-scanline_shape));
        }
    }
  
    if (DO_CCORRECTION == 1.0) pixel_out = pow(pixel_out,vec3(GAMMA_OUT));

    //Out
    return vec4(pixel_out,1.0) ; //* border(border_coords);
}

/* 
//Test masks to be used with pixel_vmask_test()
// Actually slower than pixel_vmask(), but handy for quick test
    //first element of the array is the useful size.
    const vec3 oo = vec3(0); //padding
    const vec3 mask_RGB[10] = vec3[]( vec3(3), vec3(1,0,0), vec3(0,1,0), vec3(0,0,1), oo, oo, oo, oo, oo, oo);
    const vec3 mask_GM[10] =  vec3[]( vec3(2), vec3(0,1,0), vec3(1,0,1), oo,          oo, oo, oo, oo, oo, oo);


vec3 pixel_vmask_test(vec3 color_in, vec3[10] vmask, int multiplier, float gap, vec3 white_reference, float over_white) {
    int col = int( vOutputCoord.x * params.OutputSize.x );
    vec3 vmasked;
      
    float size = (vmask[0].x + gap) * multiplier;
    
    for (int i = multiplier ; i <= size * multiplier ; i+=multiplier) {
        if (mod(col, size) < i) {
            vmasked = min (vmask[i/multiplier] + 1.0 - RGB_MASK_STRENGTH, vec3(1.0)) * color_in;
            break;
        }
    }
    
    if (over_white == 1.0) return vmasked;
        else {
            float whiteness=(white_reference.r+white_reference.g+white_reference.b)/3.0;
            whiteness-= over_white;
            whiteness=  clamp(whiteness,0.0,1.0);
            return  mix(vmasked, color_in, whiteness);
        }

}
*/


void main() {
    vec2 coords = vTexCoord;
    
    if (DO_DYNZOOM == 1.0) {
        float zoomin = 1.0 + (texture(avglum_passFeedback, vec2(0.25,0.25)    ).a/ DYNZOOM_FACTOR);
        coords = zoom(vTexCoord, zoomin);
    }
    
    vec2 coords_curved = coords;

    //Curvature: set coords_curved coordinate: (global var)
        if (DO_CURVATURE == 1.0) {
            if ((GEOM_WARP_X > 0.0) || (GEOM_WARP_Y > 0.0))
                coords_curved = Warp(coords_curved,GEOM_WARP_X,GEOM_WARP_Y);
        }
        
        if (DO_BEZEL == 1.0) {
            coords_curved = zoomout_coords(coords_curved, -BEZEL_INNER_ZOOM , 1.0);
            coords = zoomout_coords(coords, -BEZEL_INNER_ZOOM , 1.0);
        }
    
   //FIXME: bezel may need a wider border when zoomed in.
            //But for performance reasons we can wide the border in relation to the desidered reflection area.
            //by now, just wide by BEZEL_REFLECTION_AREA_SIZE defined in config.inc

    bool is_border = false;
    
    if (border_needed()) {

        float WIDEN = DO_BEZEL * BEZEL_REFLECTION_AREA_SIZE;
        bool b_is_rotated = is_rotated();
        bool is_outside_x = ((coords.x > 1.0+WIDEN ) || (coords.x < 0.0 - WIDEN) );
        bool is_outside_y = ((coords.y > 1.0+WIDEN ) || (coords.y < 0.0 - WIDEN) );
        is_border = (b_is_rotated && is_outside_y) || (!b_is_rotated && is_outside_x);
   }
   
   if (is_border)
       FragColor = mark_outer_frame(vec3(0.0));
   else
       FragColor = main_wrap(coords_curved);

       
}



#version 450

//   Modular Image Adjustment
//   Author: hunterk
//   License: Public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	uint FrameCount;
	vec4 OutputSize;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
#include "../../include/img/param_floats.h"
} global;

#include "../../include/img/helper_macros.h"

/////////////////////////////// INCLUDES ////////////////////////////////
// comment the #include and corresponding line to remove functionality //
/////////////////////////////////////////////////////////////////////////

// Flip image vertically or horizontally
#include "../../include/img/image_flip.h"

// Stretching, Zooming, Panning
#include "../../include/img/stretch_zoom_pan.h"

// Film grain effect
#include "../../include/img/film_grain.h"

// Sharp, antialiased pixels; use with linear filtering
#include "../../include/img/sharpening.h"

// Saturation and Luminance
//#include "../../include/img/sat_lum.h"
#include "../../include/img/lum_chrom.h"

// Gamma correction
// exp_gamma is basic pow function 
//#include "../../include/img/exp_gamma.h"
#include "../../include/img/gamma_srgb.h"

// Mask edges to hide unsightly garbage
#include "../../include/img/border_mask.h"

// Change the whitepoint to warmer/cooler
#include "../../include/img/white_point.h"

// Add a phosphor mask effect onto the image
#include "../../include/img/subpx_masks.h"

// Force integer scaling and custom aspect ratio
//#include "../../include/img/int_ar.h"

// Vignette; Darkens image around edges
//#include "../../include/img/vignette.h"


// Black level
// uncomment only one of the next 2 lines to set black level method
//#include "../../include/img/black_lvl.h"
#include "../../include/img/black_lvl_dogway.h"

// Brightness and Contrast control
// uncomment only one of the next 2 lines to set contract complexity;
// sigmoidal_con is advanced, bright_con is basic
#include "../../include/img/sigmoidal_con.h"
//#include "../../include/img/bright_con.h"

// Adjust color balance and tint
// uncomment only one of the next 2 lines to set color channel complexity;
// color mangler is advanced, channel mixer is basic
//#include "../../include/img/col_mangler.h"
#include "../../include/img/channel_mix.h"

// 2D screen curvature
//#include "../../include/img/gristle_warp.h"
//#include "../../include/img/lottes_warp.h"
#include "../../include/img/cgwg_warp.h"

// Rounded corners
#include "../../include/img/corner.h"

////////////////////////////// END INCLUDES //////////////////////////////

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{ 
   vec4 pos = Position;
   
// apply axis flip
   pos = flip_pos(pos);
   
   gl_Position = global.MVP * pos;
   
   vec2 coord = TexCoord.st;
   
// apply crop/zoom/pan
   coord = crop_zoom_pan(TexCoord);
   
// apply integer scaling and aspect ratio
//   coord = int_ar(coord, registers.SourceSize, registers.OutputSize);
   
   vTexCoord = coord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

//////////////////////////////// LUTS ///////////////////////////////////
// Use either 1 or 2 color-grading LUTs
// uncomment only one of the next 2 lines
//#include "../../include/img/lut1.h"
//#include "../../include/img/lut2.h"
////////////////////////////// END LUTS /////////////////////////////////

void main()
{
// declare texture coordinates
   vec2 coord = vTexCoord.xy;
   
// apply sharpening to coords
   coord = sharp(vTexCoord, registers.SourceSize);
   
// apply screen curvature
   coord = warp(coord);

// sample the texture   
   vec3 res = texture(Source, coord).rgb;

// apply grain (expects to run in gamma space)
   res = luma_grain(res, vTexCoord.xy, ia_GRAIN_STR, registers.FrameCount);
// res = rgb_grain(res, vTexCoord.xy, ia_GRAIN_STR, registers.FrameCount);

// saturation and luminance (expects to run in gamma space)
   res = sat_lum(res);
   
// contrast
   res = cntrst(res);
   
// apply first gamma transform (linearize)
// (whether LUT or gamma should come first depends on LUT)
   res = gamma_in(res);
   
// apply LUT1
//   res = lut1(res);

// apply white point adjustment
   res = white_point(res);

// black level
   res = black_level(res);
   
// channel mix
   res = channel_mix(res);
   
// overscan mask
   res = border_mask(res, coord.xy);
   
// apply LUT2 (whether LUT or gamma should come first depends on LUT)
//   res = lut2(res);
   
// apply gamma curve
   res = gamma_out(res);

// apply mask effect
   res *= mask_weights(gl_FragCoord.xy, mask_strength, mask);
   
// apply vignette effect
//   res = vignette(res, vTexCoord.xy);
   
// apply rounded corners
   res *= corner(coord);
    
   FragColor = vec4(res, 1.0);
}

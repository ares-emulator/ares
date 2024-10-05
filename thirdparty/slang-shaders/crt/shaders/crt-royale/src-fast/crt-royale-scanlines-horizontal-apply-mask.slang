#version 450

/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////

//  crt-royale: A full-featured CRT shader, with cheese.
//  Copyright (C) 2014 TroggleMonkey <trogglemonkey@gmx.com>
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program; if not, write to the Free Software Foundation, Inc., 59 Temple
//  Place, Suite 330, Boston, MA 02111-1307 USA

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	vec4 VERTICAL_SCANLINESSize;
	vec4 MASK_RESIZESize;
    float geom_d;
    float geom_R;
    float geom_cornersize;
    float geom_cornersmooth;
    float geom_x_tilt;
    float geom_y_tilt;
    float geom_overscanx;
    float geom_overscany;
    float geom_center_x;
    float geom_center_y;
    float geom_curvature;
    float geom_invert_aspect;
} params;


/*
    Geom code - a modified Geom code without CRT features made to provide
    curvature/warping/oversampling features.

    Adapted by Hyllian (2024).
*/

/*
    Copyright (C) 2010-2012 cgwg, Themaister and DOLLS

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    (cgwg gave their consent to have the original version of this shader
    distributed under the GPL in this message:

    http://board.byuu.org/viewtopic.php?p=26075#p26075

    "Feel free to distribute my shaders under the GPL. After all, the
    barrel distortion code was taken from the Curvature shader, which is
    under the GPL."
    )
    This shader variant is pre-configured with screen curvature
*/



#pragma parameter geom_curvature "Geom Curvature Toggle" 0.0 0.0 1.0 1.0
#pragma parameter geom_R "Geom Curvature Radius" 4.0 0.3 10.0 0.1
#pragma parameter geom_d "Geom Distance" 1.5 0.1 3.0 0.1
#pragma parameter geom_invert_aspect "Geom Curvature Aspect Inversion" 0.0 0.0 1.0 1.0
#pragma parameter geom_cornersize "Geom Corner Size" 0.006 0.001 1.0 0.005
#pragma parameter geom_cornersmooth "Geom Corner Smoothness" 400.0 80.0 2000.0 100.0
#pragma parameter geom_x_tilt "Geom Horizontal Tilt" 0.0 -0.5 0.5 0.01
#pragma parameter geom_y_tilt "Geom Vertical Tilt" 0.0 -0.5 0.5 0.01
#pragma parameter geom_center_x "Geom Center X" 0.0 -1.0 1.0 0.001
#pragma parameter geom_center_y "Geom Center Y" 0.0 -1.0 1.0 0.001
#pragma parameter geom_overscanx "Geom Horiz. Overscan %" 100.0 -125.0 125.0 0.5
#pragma parameter geom_overscany "Geom Vert. Overscan %" 100.0 -125.0 125.0 0.5

#define VERTICAL_SCANLINEStexture VERTICAL_SCANLINES
#define VERTICAL_SCANLINEStexture_size params.VERTICAL_SCANLINESSize.xy
#define VERTICAL_SCANLINESvideo_size params.VERTICAL_SCANLINESSize.xy
#ifdef INTEGRATED_GRAPHICS_COMPATIBILITY_MODE
	#define MASK_RESIZEtexture Source
#else
	#define MASK_RESIZEtexture MASK_RESIZE
#endif
#define MASK_RESIZEtexture_size params.MASK_RESIZESize.xy
#define MASK_RESIZEvideo_size params.MASK_RESIZESize.xy

float bloom_approx_scale_x = params.OutputSize.x / params.SourceSize.y;
float max_viewport_size_x = 1080.0*1024.0*(4.0/3.0);

/////////////////////////////  SETTINGS MANAGEMENT  ////////////////////////////

#include "bind-shader-params.h"

///////////////////////////////  VERTEX INCLUDES  ///////////////////////////////

#include "scanline-functions.h"
#include "phosphor-mask-resizing.h"

///////////////////////////////////  HELPERS  //////////////////////////////////

// Macros.
#define FIX(c) max(abs(c), 1e-5);
#define PI 3.141592653589


// aspect ratio
vec2 aspect     = vec2(params.geom_invert_aspect > 0.5 ? (0.75, 1.0) : (1.0, 0.75));
vec2 overscan   = vec2(1.01, 1.01);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 video_uv;
layout(location = 1) out vec2 scanline_texture_size_inv;
layout(location = 2) out vec4 mask_tile_start_uv_and_size;
layout(location = 3) out vec2 mask_tiles_per_screen;
layout(location = 4) out vec2 sinangle;
layout(location = 5) out vec2 cosangle;
layout(location = 6) out vec3 stretch;
layout(location = 7) out float R_d_cx_cy;
layout(location = 8) out float d2;

float intersect(vec2 xy)
{
    float A = dot(xy,xy) + params.geom_d*params.geom_d;
    float B = 2.0*(params.geom_R*(dot(xy,sinangle)-params.geom_d*cosangle.x*cosangle.y)-params.geom_d*params.geom_d);
    float C = params.geom_d*params.geom_d + 2.0*params.geom_R*params.geom_d*cosangle.x*cosangle.y;
    
    return (-B-sqrt(B*B-4.0*A*C))/(2.0*A);
}

vec2 bkwtrans(vec2 xy)
{
    float c     = intersect(xy);
    vec2 point  = (vec2(c, c)*xy - vec2(-params.geom_R, -params.geom_R)*sinangle) / vec2(params.geom_R, params.geom_R);
    vec2 poc    = point/cosangle;
    
    vec2 tang   = sinangle/cosangle;
    float A     = dot(tang, tang) + 1.0;
    float B     = -2.0*dot(poc, tang);
    float C     = dot(poc, poc) - 1.0;
    
    float a     = (-B + sqrt(B*B - 4.0*A*C))/(2.0*A);
    vec2 uv     = (point - a*sinangle)/cosangle;
    float r     = FIX(params.geom_R*acos(a));
    
    return uv*r/sin(r/params.geom_R);
}

vec2 fwtrans(vec2 uv)
{
    float r = FIX(sqrt(dot(uv,uv)));
    uv *= sin(r/params.geom_R)/r;
    float x = 1.0-cos(r/params.geom_R);
    float D = params.geom_d/params.geom_R + x*cosangle.x*cosangle.y+dot(uv,sinangle);
    
    return params.geom_d*(uv*cosangle-x*sinangle)/D;
}

vec3 maxscale()
{
    vec2 c  = bkwtrans(-params.geom_R * sinangle / (1.0 + params.geom_R/params.geom_d*cosangle.x*cosangle.y));
    vec2 a  = vec2(0.5,0.5)*aspect;
    
    vec2 lo = vec2(fwtrans(vec2(-a.x,  c.y)).x,
                   fwtrans(vec2( c.x, -a.y)).y)/aspect;

    vec2 hi = vec2(fwtrans(vec2(+a.x,  c.y)).x,
                   fwtrans(vec2( c.x, +a.y)).y)/aspect;
    
    return vec3((hi+lo)*aspect*0.5,max(hi.x-lo.x,hi.y-lo.y));
}

void main()
{
   gl_Position = global.MVP * Position;

    video_uv = TexCoord * vec2(1.0001) - vec2(params.geom_center_x, params.geom_center_y);


    // Precalculate a bunch of useful values we'll need in the fragment
    // shader.
    sinangle    = sin(vec2(params.geom_x_tilt, params.geom_y_tilt));
    cosangle    = cos(vec2(params.geom_x_tilt, params.geom_y_tilt));
    stretch     = maxscale();

    d2        = params.geom_d * params.geom_d;
    R_d_cx_cy = params.geom_R * params.geom_d * cosangle.x * cosangle.y;

    scanline_texture_size_inv = vec2(1.0, 1.0)/VERTICAL_SCANLINEStexture_size;

    //  Get a consistent name for the final mask texture size.  Sample mode 0
    //  uses the manually resized mask, but ignore it if we never resized.
    #ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
        vec2 mask_resize_texture_size = MASK_RESIZEtexture_size;
        vec2 mask_resize_video_size = MASK_RESIZEvideo_size;
    #endif
    //  Compute mask tile dimensions, starting points, etc.:
    mask_tile_start_uv_and_size = get_mask_sampling_parameters(mask_resize_texture_size, mask_resize_video_size, params.OutputSize.xy, mask_tiles_per_screen);
}

#pragma stage fragment
layout(location = 0) in vec2 video_uv;
layout(location = 1) in vec2 scanline_texture_size_inv;
layout(location = 2) in vec4 mask_tile_start_uv_and_size;
layout(location = 3) in vec2 mask_tiles_per_screen;
layout(location = 4) in vec2 sinangle;
layout(location = 5) in vec2 cosangle;
layout(location = 6) in vec3 stretch;
layout(location = 7) in float R_d_cx_cy;
layout(location = 8) in float d2;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D VERTICAL_SCANLINES;
#ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
layout(set = 0, binding = 4) uniform sampler2D MASK_RESIZE;
#endif


float intersect(vec2 xy, float d2, float R_d_cx_cy)
{
    float A = dot(xy,xy) + d2;
    float B = 2.0 * ( params.geom_R * dot(xy, sinangle) - R_d_cx_cy - d2);
    float C = d2 + 2.0 * R_d_cx_cy;
    
    return (-B-sqrt(B*B-4.0*A*C))/(2.0*A);
}

vec2 bkwtrans(vec2 xy, float d2, float R_d_cx_cy)
{
    float c     = intersect(xy, d2, R_d_cx_cy);
    vec2 point  = (vec2(c, c)*xy - vec2(-params.geom_R, -params.geom_R)*sinangle) / vec2(params.geom_R, params.geom_R);
    vec2 poc    = point/cosangle;
    
    vec2 tang   = sinangle/cosangle;
    float A     = dot(tang, tang) + 1.0;
    float B     = -2.0*dot(poc, tang);
    float C     = dot(poc, poc) - 1.0;
    
    float a     = (-B + sqrt(B*B - 4.0*A*C))/(2.0*A);
    vec2 uv     = (point - a*sinangle)/cosangle;
    float r     = FIX(params.geom_R*acos(a));
    
    return uv*r/sin(r/params.geom_R);
}

vec2 transform(vec2 coord, float d2, float R_d_cx_cy)
{
    coord = (coord - vec2(0.5, 0.5))*aspect*stretch.z + stretch.xy;
    
    return (bkwtrans(coord, d2, R_d_cx_cy) /
        vec2(params.geom_overscanx / 100.0, params.geom_overscany / 100.0)/aspect + vec2(0.5, 0.5));
}

float corner(vec2 coord)
{
    coord = (coord - vec2(0.5)) * vec2(params.geom_overscanx / 100.0, params.geom_overscany / 100.0) + vec2(0.5, 0.5);
    coord = min(coord, vec2(1.0) - coord) * aspect;
    vec2 cdist = vec2(params.geom_cornersize);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));
    
      return clamp((cdist.x - dist)*params.geom_cornersmooth, 0.0, 1.0);
}


//////////////////////////////  FRAGMENT INCLUDES  //////////////////////////////

void main()
{
    // Texture coordinates of the texel containing the active pixel.
    vec2  uv   = video_uv;
    float cval = 1.0;

    if (params.geom_curvature > 0.5)
    {
        uv   = transform(video_uv, d2, R_d_cx_cy);
        cval = corner(uv);
    }


    //  This pass: Sample (misconverged?) scanlines to the final horizontal
    //  resolution, apply halation (bouncing electrons), and apply the phosphor
    //  mask.  Fake a bloom if requested.  Unless we fake a bloom, the output
    //  will be dim from the scanline auto-dim, mask dimming, and low gamma.

    //  Horizontally sample the current row (a vertically interpolated scanline)
    //  and account for horizontal convergence offsets, given in units of texels.
    vec3 scanline_color_dim = sample_rgb_scanline_horizontal(VERTICAL_SCANLINEStexture, uv, VERTICAL_SCANLINEStexture_size, scanline_texture_size_inv);

    //  Sample the phosphor mask:
    vec2 tile_uv_wrap = video_uv * mask_tiles_per_screen;

    vec2 mask_tex_uv = convert_phosphor_tile_uv_wrap_to_tex_uv(tile_uv_wrap, mask_tile_start_uv_and_size);

    vec3 phosphor_mask_sample;

    //  Sample the resized mask, and avoid tiling artifacts:
    phosphor_mask_sample = texture(MASK_RESIZEtexture, mask_tex_uv).rgb;

    //  Apply the phosphor mask:
    vec3 phosphor_emission_dim = scanline_color_dim * phosphor_mask_sample * vec3(cval);

    phosphor_emission_dim *= step(0.0, fract(uv.y)); // Discard off limit pixels

    FragColor = vec4(phosphor_emission_dim, 1.0);
}

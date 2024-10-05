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
	uint FrameCount;
} params;

/////////////////////////////  SETTINGS MANAGEMENT  ////////////////////////////

#include "bind-shader-params.h"

//////////////////////////////////  INCLUDES  //////////////////////////////////

#include "phosphor-mask-resizing.h"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 src_tex_uv_wrap;
layout(location = 1) out vec2 resize_magnification_scale;

void main()
{
    gl_Position = global.MVP * Position;
    vec2 tex_uv = TexCoord;

    //  First estimate the viewport size (the user will get the wrong number of
    //  triads if it's wrong and mask_specify_num_triads is 1.0/true).
//    float viewport_y              = params.OutputSize.y / mask_resize_viewport_scale.y;
//    float aspect_ratio            = geom_aspect_ratio_x / geom_aspect_ratio_y;
//    vec2  estimated_viewport_size = vec2(viewport_y * aspect_ratio, viewport_y);

    //  Estimate the output size of MASK_RESIZE (the next pass).  The estimated
    //  x component shouldn't matter, because we're not using the x result, and
    //  we're not swearing it's correct (if we did, the x result would influence
    //  the y result to maintain the tile aspect ratio).
//    vec2 estimated_mask_resize_output_size = vec2(params.OutputSize.y * aspect_ratio, params.OutputSize.y);

    //  First estimate the viewport size (the user will get the wrong number of
    //  triads if it's wrong and mask_specify_num_triads is 1.0/true).
    vec2  estimated_viewport_size = params.OutputSize.xy / mask_resize_viewport_scale.yy;

    //  Estimate the output size of MASK_RESIZE (the next pass).  The estimated
    //  x component shouldn't matter, because we're not using the x result, and
    //  we're not swearing it's correct (if we did, the x result would influence
    //  the y result to maintain the tile aspect ratio).
    vec2 estimated_mask_resize_output_size = params.OutputSize.xy;

    //  Find the final intended [y] size of our resized phosphor mask tiles,
    //  then the tile size for the current pass (resize y only):
    vec2 mask_resize_tile_size = get_resized_mask_tile_size(estimated_viewport_size, estimated_mask_resize_output_size, false);
    vec2 pass_output_tile_size = vec2(min(mask_resize_src_lut_size.x, params.OutputSize.x), mask_resize_tile_size.y);

    //  We'll render resized tiles until filling the output FBO or meeting a
    //  limit, so compute [wrapped] tile uv coords based on the output uv coords
    //  and the number of tiles that will fit in the FBO.
    vec2 output_tiles_this_pass = params.OutputSize.xy / pass_output_tile_size;
    vec2 output_video_uv        = tex_uv;
    vec2 tile_uv_wrap           = output_video_uv * output_tiles_this_pass;

    //  The input LUT is just a single mask tile, so texture uv coords are the
    //  same as tile uv coords (save fract() for the fragment shader).  The
    //  magnification scale is also straightforward:
    src_tex_uv_wrap            = tile_uv_wrap;
    resize_magnification_scale = pass_output_tile_size / mask_resize_src_lut_size;
}

#pragma stage fragment
layout(location = 0) in vec2 src_tex_uv_wrap;
layout(location = 1) in vec2 resize_magnification_scale;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D mask_grille_texture_small;
layout(set = 0, binding = 4) uniform sampler2D mask_slot_texture_small;
layout(set = 0, binding = 5) uniform sampler2D mask_shadow_texture_small;

void main()
{
    //  Resize the input phosphor mask tile to the final vertical size it will
    //  appear on screen.  Keep 1x horizontal size if possible (IN.output_size
    //  >= mask_resize_src_lut_size), and otherwise linearly sample horizontally
    //  to fit exactly one tile.  Lanczos-resizing the phosphor mask achieves
    //  much sharper results than mipmapping, and vertically resizing first
    //  minimizes the total number of taps required.  We output a number of
    //  resized tiles >= mask_resize_num_tiles for easier tiled sampling later.
    //vec2 src_tex_uv_wrap = src_tex_uv_wrap;
    #ifdef PHOSPHOR_MASK_MANUALLY_RESIZE
    //  Discard unneeded fragments in case our profile allows real branches.
    vec2 tile_uv_wrap = src_tex_uv_wrap;

    if(tile_uv_wrap.y <= mask_resize_num_tiles)
    {
        float src_dy     = 1.0/mask_resize_src_lut_size.y;
        vec2  src_tex_uv = fract(src_tex_uv_wrap);
        vec3  pixel_color;

        if(mask_type < 0.5)
        {
            pixel_color = downsample_vertical_sinc_tiled(mask_grille_texture_small, src_tex_uv, mask_resize_src_lut_size, src_dy, resize_magnification_scale.y, 1.0);
        }
        else if(mask_type < 1.5)
        {
            pixel_color = downsample_vertical_sinc_tiled(mask_slot_texture_small, src_tex_uv, mask_resize_src_lut_size, src_dy, resize_magnification_scale.y, 1.0);
        }
        else
        {
            pixel_color = downsample_vertical_sinc_tiled(mask_shadow_texture_small, src_tex_uv, mask_resize_src_lut_size, src_dy, resize_magnification_scale.y, 1.0);
        }
            //  The input LUT was linear RGB, and so is our output:
            FragColor = vec4(pixel_color, 1.0);
        }
        else
        {
            discard;
        }
    #else
        discard;
        FragColor = vec4(1.0);
	#endif
}

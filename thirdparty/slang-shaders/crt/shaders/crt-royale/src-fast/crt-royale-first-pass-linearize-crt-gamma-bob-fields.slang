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

//////////////////////////////////  INCLUDES  //////////////////////////////////

#include "bind-shader-params.h"
#include "scanline-functions.h"

#define GAMMA_IN(color)    pow(color, vec3(crt_gamma))

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 tex_uv;
layout(location = 1) out vec2 uv_step;
layout(location = 2) out float interlaced;

void main()
{
   gl_Position = global.MVP * Position;
   tex_uv      = TexCoord * 1.00001;
   uv_step     = vec2(1.0)/params.SourceSize.xy;
   
    //  Detect interlacing: 1.0 = true, 0.0 = false.
    vec2 _video_size = params.SourceSize.xy;
    interlaced       = float(is_interlaced(_video_size.y));
}

#pragma stage fragment
#pragma format R8G8B8A8_SRGB
layout(location = 0) in vec2 tex_uv;
layout(location = 1) in vec2 uv_step;
layout(location = 2) in float interlaced;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
#define input_texture Source

void main()
{
    //  Linearize the input based on CRT gamma and bob interlaced fields.
    //  Bobbing ensures we can immediately blur without getting artifacts.
    //  Note: TFF/BFF won't matter for sources that double-weave or similar.
    if(bool(interlace_detect))
    {
        //  Sample the current line and an average of the previous/next line;
        //  tex2D_linearize will decode CRT gamma.  Don't bother branching:
        vec2 v_step = vec2(0.0, uv_step.y);

        vec3 curr_line = GAMMA_IN(texture(input_texture, tex_uv         ).rgb);
        vec3 last_line = GAMMA_IN(texture(input_texture, tex_uv - v_step).rgb);
        vec3 next_line = GAMMA_IN(texture(input_texture, tex_uv + v_step).rgb);

        vec3 interpolated_line = 0.5 * (last_line + next_line);

        //  If we're interlacing, determine which field curr_line is in:
        float modulus         = interlaced + 1.0;
        float field_offset    = mod(params.FrameCount + global.interlace_bff, modulus);
        float curr_line_texel = tex_uv.y * params.SourceSize.y;

        //  Use under_half to fix a rounding bug around exact texel locations.
        float line_num_last = floor(curr_line_texel - under_half);
        float wrong_field   = mod(line_num_last + field_offset, modulus);

        //  Select the correct color, and output the result:
        vec3 color = mix(curr_line, interpolated_line, wrong_field);
        FragColor  = vec4(color, 1.0);
    }
    else
    {
        FragColor = vec4(GAMMA_IN(texture(input_texture, tex_uv).rgb), 1.0);
    }
}

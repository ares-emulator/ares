#version 450

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Copyright (C) 2017 - Brad Parker                                      //
//                                                                       //
// This program is free software: you can redistribute it and/or modify  //
// it under the terms of the GNU General Public License as published by  //
// the Free Software Foundation, either version 3 of the License, or     //
// (at your option) any later version.                                   //
//                                                                       //
// This program is distributed in the hope that it will be useful,       //
// but WITHOUT ANY WARRANTY; without even the implied warranty of        //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
// GNU General Public License for more details.                          //
//                                                                       //
// You should have received a copy of the GNU General Public License     //
// along with this program.  If not, see <http://www.gnu.org/licenses/>. //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// Largest integer scale of input video that will fit in the current output (y axis would typically be limiting on widescreens)
#define video_scale         floor(params.OutputSize.y * params.SourceSize.w)

// Size of the scaled video
//#define scaled_video_out    (params.SourceSize.xy * vec2(video_scale))

//it's... half a pixel
#define half_pixel          (vec2(0.5) * params.OutputSize.zw)   

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 dot_size;
layout(location = 2) out vec2 one_texel;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
	vec2 scaled_video_out = (params.SourceSize.xy * vec2(video_scale));
	dot_size = params.SourceSize.zw;
	one_texel = 1.0 / (params.SourceSize.xy * video_scale);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 dot_size;
layout(location = 2) in vec2 one_texel;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D MASK;

#define maskSize vec2(2., floor(params.OutputSize.y / params.SourceSize.y + 0.000001))

#define curr_rgb  texture(Source, vTexCoord)
#define mask_rgb  texture(MASK, fract((vTexCoord.xy * params.OutputSize.xy) / maskSize))

void main()
{
   FragColor = mask_rgb * curr_rgb;
}
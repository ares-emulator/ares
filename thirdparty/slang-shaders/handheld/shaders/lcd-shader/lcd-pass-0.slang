#version 450

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// LCD Shader v0.0.1                                                     //
//                                                                       //
// Copyright (C) 2013 Harlequin : unknown92835@gmail.com                 //
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
	uint FrameCount;    float response_time;
} params;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//config                                                                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Simulate response time
// Higher values result in longer color transition periods - [0, 1]
#pragma parameter response_time "LCD Response Time" 0.333 0.0 0.777 0.111
#define response_time params.response_time

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//vertex definitions                                                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define video_scale 		floor(params.OutputSize.y * params.SourceSize.w)		//largest integer scale of input video that will fit in the current output (y axis would typically be limiting on widescreens)
#define scaled_video_out	(params.SourceSize.xy * video_scale)				//size of the scaled video
#define half_pixel		(0.5 * params.OutputSize.zw)					//it's... half a pixel

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//vertex shader                                                                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float cell_height;
layout(location = 2) out float texel_height;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   cell_height = params.SourceSize.w;
   texel_height = 1.0 / (params.SourceSize.y * video_scale);
}

#pragma stage fragment

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment definitions                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define curr_rgb  texture(Source, vTexCoord).rgb
#define prev0_rgb texture(OriginalHistory1, vTexCoord).rgb
#define prev1_rgb texture(OriginalHistory2, vTexCoord).rgb
#define prev2_rgb texture(OriginalHistory3, vTexCoord).rgb
#define prev3_rgb texture(OriginalHistory4, vTexCoord).rgb
#define prev4_rgb texture(OriginalHistory5, vTexCoord).rgb
#define prev5_rgb texture(OriginalHistory6, vTexCoord).rgb
#define prev6_rgb texture(OriginalHistory7, vTexCoord).rgb

#define line_alpha 0.5		//arbitrary  0<a<1 value used to distinguish vertical line fragments from the border and cell fragments later one
				//ANY CHANGE TO THIS SHOULD BE REPEATED IN lcd_pass_3 SO IT CAN IDENTIFY LINE FRAGMENTS PROPERLY

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment shader                                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float cell_height;
layout(location = 2) in float texel_height;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D OriginalHistory2;
layout(set = 0, binding = 5) uniform sampler2D OriginalHistory3;
layout(set = 0, binding = 6) uniform sampler2D OriginalHistory4;
layout(set = 0, binding = 7) uniform sampler2D OriginalHistory5;
layout(set = 0, binding = 8) uniform sampler2D OriginalHistory6;
layout(set = 0, binding = 9) uniform sampler2D OriginalHistory7;

void main()
{
  //motion blur

    vec3 input_rgb = curr_rgb;
    input_rgb += (prev0_rgb - input_rgb) * response_time;
    input_rgb += (prev1_rgb - input_rgb) * pow(response_time, 2.0);
    input_rgb += (prev2_rgb - input_rgb) * pow(response_time, 3.0);
    input_rgb += (prev3_rgb - input_rgb) * pow(response_time, 4.0);
    input_rgb += (prev4_rgb - input_rgb) * pow(response_time, 5.0);
    input_rgb += (prev5_rgb - input_rgb) * pow(response_time, 6.0);
    input_rgb += (prev6_rgb - input_rgb) * pow(response_time, 7.0);

    vec4 out_color = vec4(input_rgb, 1.0);

  //add horizontal lines

    bool is_on_line = bool(mod(vTexCoord.y, cell_height) > texel_height);
    FragColor = vec4( (out_color.rgb * float(is_on_line)), (out_color.a - (line_alpha * float(!is_on_line))) );
}

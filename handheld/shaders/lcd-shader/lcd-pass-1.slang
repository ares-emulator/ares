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
	uint FrameCount;
	float cell_scale, triad_color_0_r, triad_color_0_g, triad_color_0_b,
	   triad_color_1_r, triad_color_1_g, triad_color_1_b, triad_color_2_r,
	   triad_color_2_g, triad_color_2_b;
} params;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//config                                                                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma parameter cell_scale "LCD Cell Scale" 1.0 1.0 5.0 1.0
#pragma parameter triad_color_0_r "LCD Red Triad Color R" 1.0 0.0 1.0 0.1
#pragma parameter triad_color_0_g "LCD Red Triad Color G" 0.0 0.0 1.0 0.1
#pragma parameter triad_color_0_b "LCD Red Triad Color B" 1.0 0.0 1.0 0.1
#pragma parameter triad_color_1_r "LCD Green Triad Color R" 1.0 0.0 1.0 0.1
#pragma parameter triad_color_1_g "LCD Green Triad Color G" 1.0 0.0 1.0 0.1
#pragma parameter triad_color_1_b "LCD Green Triad Color B" 0.0 0.0 1.0 0.1
#pragma parameter triad_color_2_r "LCD Blue Triad Color R" 0.0 0.0 1.0 0.1
#pragma parameter triad_color_2_g "LCD Blue Triad Color G" 1.0 0.0 1.0 0.1
#pragma parameter triad_color_2_b "LCD Blue Triad Color B" 1.0 0.0 1.0 0.1

#define cell_scale params.cell_scale
#define triad_color_0_r params.triad_color_0_r
#define triad_color_0_g params.triad_color_0_g
#define triad_color_0_b params.triad_color_0_b
#define triad_color_1_r params.triad_color_1_r
#define triad_color_1_g params.triad_color_1_g
#define triad_color_1_b params.triad_color_1_b
#define triad_color_2_r params.triad_color_2_r
#define triad_color_2_g params.triad_color_2_g
#define triad_color_2_b params.triad_color_2_b

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//vertex shader                                                                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float dX;
layout(location = 2) out float two_dX;
layout(location = 3) out float three_dX;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   
   float texel_width = params.SourceSize.z;
   dX = texel_width;
   two_dX = 2.0 * texel_width;
   three_dX = 3.0 * texel_width;
}

#pragma stage fragment

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment definitions                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define triad_color_0 vec3(triad_color_0_r, triad_color_0_g, triad_color_0_b)	//magenta
#define triad_color_1 vec3(triad_color_1_r, triad_color_1_g, triad_color_1_b)	//yellow
#define triad_color_2 vec3(triad_color_2_r, triad_color_2_g, triad_color_2_b)	//cyan

//#define triad_color_0 vec3(0.0, 0.0, 1.0)	//blue
//#define triad_color_1 vec3(1.0, 0.0, 0.0)	//red
//#define triad_color_2 vec3(0.0, 1.0, 0.0)	//green

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment shader                                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float dX;
layout(location = 2) in float two_dX;
layout(location = 3) in float three_dX;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
  //use modulo to deterimine the current subcell location and apply proper color

    float modX = mod(vTexCoord.x, cell_scale * three_dX);

    vec3 subpixel_color = (modX < cell_scale * dX)		? triad_color_0 : 
			    (modX < cell_scale * two_dX)	? triad_color_1 : triad_color_2;


  //use color darkening with input texture to determine the final color of the subpixel
  //color darkening: the minimum value for each color component between the LCD cell and the input image is selected
  //ex. LCD cell subpixel is magenta (1.0, 1.0, 0.0) and the current video texel is red (1.0, 0.0, 0.0)...
  //...the result will be the minimum of each component: result.rgb = min(1.0, 1.0), min(1.0, 0.0), min(0.0, 0.0) = (1.0, 0.0, 0.0) = red

    vec4 out_color = texture(Source, vTexCoord);

    out_color.rgb = vec3( min(out_color.r, subpixel_color.r), 
			    min(out_color.g, subpixel_color.g), 
			    min(out_color.b, subpixel_color.b) );

   FragColor = out_color;
}

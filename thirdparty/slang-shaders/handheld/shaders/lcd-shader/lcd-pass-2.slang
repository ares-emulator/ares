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
	float LCD_to_input_ratio, bg_tint_r, bg_tint_g, bg_tint_b;
} params;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//config                                                                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma parameter LCD_to_input_ratio "LCD to Input Ratio" 0.9 0.0 1.0 0.01
//the ratio of blending between the LCD pixels and the input image, higher values result in stronger LCD magenta/yellow/cyan bands - [0, 1]
#pragma parameter bg_tint_r "LCD Background Tint (Red)" 0.0 0.0 1.0 0.01
#pragma parameter bg_tint_g "LCD Background Tint (Green)" 0.0 0.0 1.0 0.01
#pragma parameter bg_tint_b "LCD Background Tint (Blue)" 0.0 0.0 1.0 0.01

#define LCD_to_input_ratio params.LCD_to_input_ratio
#define bg_tint_r params.bg_tint_r
#define bg_tint_g params.bg_tint_g
#define bg_tint_b params.bg_tint_b

vec3 bg_tint = vec3(bg_tint_r, bg_tint_g, bg_tint_b);	//color to tint the background image in RGB format - each color component ranges from 0.0 (none) to 1.0 (fully saturated)

#define saturate(c) clamp(c, 0., 1.)

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

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment definitions                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define is_border_texel bool(out_color.a == 0.0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment shader                                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D PASS1;
layout(set = 0, binding = 4) uniform sampler2D BACKGROUND;

void main()
{
  //sample all relevant textures

    vec4 lcd_color   = texture(Source, vTexCoord);		//color darkened pixels from the previous pass
    vec4 input_color = texture(PASS1, vTexCoord);	//scaled input video with vertical lines from first pass
    vec4 bg_color    = texture(BACKGROUND, vTexCoord);		//background image

  //overlay the LCD image onto the input image to brighten the output and dampen the LCD cell colors

    vec4 out_color = vec4( (lcd_color.rgb * LCD_to_input_ratio) + (input_color.rgb * (1.0 - LCD_to_input_ratio)), input_color.a);

  //tint the background image and apply it to the output color if the current fragment is located in the border region

    bg_color.rgb = saturate( vec3( 				//allows for highlights, bg_color = bg_tint when the background image color is 0.5 gray
	bg_tint.r + mix(-1.0, 1.0, bg_color.r), 
	bg_tint.g + mix(-1.0, 1.0, bg_color.g), 
	bg_tint.b + mix(-1.0, 1.0, bg_color.b) ) );

    out_color.rgb = (bg_color.rgb * float(is_border_texel)) + (out_color.rgb * float(!is_border_texel));
    FragColor = out_color;
}

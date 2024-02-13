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
	float LCD_blending, original_blending;
} params;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//config                                                                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//values related to color blending on the darkened vertical lines of the output image

//contribution of LCD colors to the blended output, higher values apply more color - [0.0, 0.5]
#pragma parameter LCD_blending "LCD Blending" 0.2 0.0 0.5 0.01
//contribution of the original input colors to the blended output, higher values apply more color - [0.0, 0.5]
#pragma parameter original_blending "Original Blending" 0.2 0.0 0.5 0.01

#define LCD_blending params.LCD_blending
#define original_blending params.original_blending

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
layout(location = 1) out vec2 tex_coord_1;
layout(location = 2) out vec2 tex_coord_2;
layout(location = 3) out vec2 lower_bound;
layout(location = 4) out vec2 upper_bound;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   
   vec2 texel = params.SourceSize.zw;	//size of one texel
   
   tex_coord_1 = vTexCoord + vec2(0.0, texel.y);	//down
	tex_coord_2 = vTexCoord + vec2(0.0, -texel.y);	//up
	lower_bound = vec2(0.0);				//lower texture bounds
	upper_bound = texel * (params.OutputSize.xy - 2.0);		//upper texture bounds
}

#pragma stage fragment

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment definitions                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define epsilon 0.1
#define line_alpha 0.5		//arbitrary  0<a<1 value used to distinguish vertical line fragments from the border and cell fragments later one
				//ANY CHANGE TO THIS SHOULD BE REPEATED IN lcd_pass_0 SO IT CAN PROPERLY SET THE ALPHA VALUES

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment functions                                                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//a simple blur technique that softens harsh color transitions and applies the results to fragmetns that lie on the darkened vertical lines of the image
//specialized to sample from the original input as well as the LCD overlay and allow mixing between the results

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragment shader                                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 tex_coord_1;
layout(location = 2) in vec2 tex_coord_2;
layout(location = 3) in vec2 lower_bound;
layout(location = 4) in vec2 upper_bound;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D PASS1;

vec4 simple_blur(vec4 COLOR)
{
  //clamp the blur coords to the input texture size so it doesn't attempt to sample off the texture (it'll retrieve vec4(0.,0.,0.,0.) and darken the edges otherwise)

    vec2 new_tex_coord_1 = clamp(tex_coord_1, lower_bound, upper_bound);
    vec2 new_tex_coord_2 = clamp(tex_coord_2, lower_bound, upper_bound);

  //sample adjacent texels based on the coordinates above

    vec4 adjacent_texel_1 = texture(Source, new_tex_coord_1);	//LCD, down
    vec4 adjacent_texel_2 = texture(Source, new_tex_coord_2);	//LCD, up

    vec4 adjacent_texel_3 = texture(PASS1, new_tex_coord_1);	//original, down
    vec4 adjacent_texel_4 = texture(PASS1, new_tex_coord_2);	//original, up

  //sum the differences between neighboring texels and apply modifiers

    vec3 LCD_color = ( (COLOR.rgb - adjacent_texel_1.rgb) + 
			 (COLOR.rgb - adjacent_texel_2.rgb) ) * LCD_blending;

    vec3 original_color = ( (COLOR.rgb - adjacent_texel_3.rgb) + 
			      (COLOR.rgb - adjacent_texel_4.rgb) ) * original_blending;

  //subtract the values calculated above from the input color
    COLOR.rgb -= original_color + LCD_color;

  //return new value
    return COLOR;
}

void main()
{
  //sample input texture

    vec4 out_color = texture(Source, vTexCoord);


  //determine if the current fragment is located on the darkened vertical lines

    bool is_on_line = bool(abs(out_color.a - line_alpha) < epsilon);


  //apply simple_blur to line fragments, otherwise keep out_color equal to the input value

    out_color = (simple_blur(out_color) * float(is_on_line)) + (out_color * float(!is_on_line));
    FragColor = out_color;
}

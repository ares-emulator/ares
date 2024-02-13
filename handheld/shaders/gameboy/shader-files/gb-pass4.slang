#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	vec4 PassOutputSize1;
	float contrast;
	float screen_light;
	float pixel_opacity;
	float bg_smoothing;
	float shadow_opacity;
	float shadow_offset_x;
	float shadow_offset_y;
	float screen_offset_x;
	float screen_offset_y;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

////////////////////////////////////////////////////////////////////////////////
// Config                                                                     //
////////////////////////////////////////////////////////////////////////////////

// Useful to fine-tune the colors.
// Higher values make the "black" color closer to black - [0, 1] [DEFAULT: 0.95]
#pragma parameter contrast "Contrast" 0.95 0.0 1.0 0.05

// Controls the ambient light of the screen. 
// Lower values darken the screen - [0, 2] [DEFAULT: 1.00]
#pragma parameter screen_light "Ambient Screen Light" 1.0 0.0 2.0 0.05

// Controls the opacity of the dot-matrix pixels. 
// Lower values make pixels more transparent - [0, 1] [DEFAULT: 1.00]
#pragma parameter pixel_opacity "Pixel Opacity" 1.0 0.01 1.0 0.01

// Higher values suppress changes in background color directly beneath
// the foreground to improve image clarity - [0, 1] [DEFAULT: 0.75]
#pragma parameter bg_smoothing "Background Smooth" 0.75 0.0 1.0 0.05

// How strongly shadows affect the background
// Higher values darken the shadows - [0, 1] [DEFAULT: 0.55]
#pragma parameter shadow_opacity "Shadow Opacity" 0.55 0.01 1.0 0.01

// How far the shadow should be shifted to the
// right in pixels - [-infinity, infinity] [DEFAULT: 1.0]
#pragma parameter shadow_offset_x "Shadow Offset Horiz" 1.0 -5.0 5.0 0.5

// How far the shadow should be shifted
// down in pixels - [-infinity, infinity] [DEFAULT: 1.5]
#pragma parameter shadow_offset_y "Shadow Offset Vert" 1.0 -5.0 5.0 0.5

// Screen offset - [-infinity, infinity] [DEFAULT: 0]
#pragma parameter screen_offset_x "Screen Offset Horiz" 0.0 -5.0 5.0 0.5

// Screen offset - [-infinity, infinity] [DEFAULT: 0]
#pragma parameter screen_offset_y "Screen Offset Vert" 0.0 -5.0 5.0 0.5    

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Gameboy Classic Shader v0.2.2                                         //
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

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 texel;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord * 1.0001;

    texel = registers.SourceSize.zw;
}

////////////////////////////////////////////////////////////////////////////////
// Fragment definitions                                                       //
////////////////////////////////////////////////////////////////////////////////

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 texel;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;
layout(set = 0, binding = 2) uniform sampler2D PassOutput1;
layout(set = 0, binding = 3) uniform sampler2D BACKGROUND;
layout(set = 0, binding = 4) uniform sampler2D COLOR_PALETTE;

#define bg_color texture(COLOR_PALETTE, vec2(0.25, 0.5)) 

// Sample the background color from the palette
#define shadow_alpha (registers.contrast * registers.shadow_opacity)

// Offset for the shadow
#define shadow_offset vec2(registers.shadow_offset_x * texel.x, registers.shadow_offset_y * texel.y)    

// Offset for the entire screen
#define screen_offset vec2(registers.screen_offset_x * texel.x, registers.screen_offset_y * texel.y)   

////////////////////////////////////////////////////////////////////////////////
// Fragment shader                                                            //
////////////////////////////////////////////////////////////////////////////////

void main()
{
    vec2 tex = floor(registers.PassOutputSize1.xy * vTexCoord);
    tex = (tex + 0.5) * registers.PassOutputSize1.zw;
    
    // Sample all the relevant textures
    vec4 foreground = texture(PassOutput1, tex - screen_offset);
    vec4 background = texture(BACKGROUND, vTexCoord);
    vec4 shadows    = texture(Source, vTexCoord - (shadow_offset + screen_offset));
    vec4 background_color = bg_color;

    // Foreground and background are blended with the background color
    foreground *= bg_color;
    
    float bg_test = 0.0;
    if ( foreground.a > 0.0 )
        bg_test = 1.0; 

    background -= (background - 0.5) * registers.bg_smoothing * bg_test;

    // Allows for highlights,
    // background = bg_color when the background color is 0.5 gray
    background.rgb = clamp(
        vec3( 
            bg_color.r + mix(-1.0, 1.0, background.r), 
            bg_color.g + mix(-1.0, 1.0, background.g), 
            bg_color.b + mix(-1.0, 1.0, background.b)
        ), 
        0.0, 1.0
    );

    // Shadows are alpha blended with the background
    vec4 out_color = (shadows * shadows.a * shadow_alpha) + (background * (1 - shadows.a * shadow_alpha));  

    // Foreground is alpha blended with the shadowed background
    out_color = (foreground * foreground.a * registers.contrast) + (out_color * (registers.screen_light - foreground.a * registers.contrast * registers.pixel_opacity));
    
    FragColor = out_color;
}

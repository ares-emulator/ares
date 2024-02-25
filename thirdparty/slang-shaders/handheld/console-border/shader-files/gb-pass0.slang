#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	vec4 OriginalHistorySize1;
	float baseline_alpha;
   float grey_balance;
	float response_time;
	float video_scale;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

////////////////////////////////////////////////////////////////////////////////
// Config                                                                     //
////////////////////////////////////////////////////////////////////////////////

// The alpha value of dots in their "off" state
// Does not affect the border region of the screen - [0, 1]
#pragma parameter baseline_alpha "Baseline Alpha" 0.05 0.0 1.0 0.01

// Fine-tune the balance between the different shades of grey
#pragma parameter grey_balance "Grey Balance" 2.7 2.0 4.0 0.1

// Simulate response time
// Higher values result in longer color transition periods - [0, 1]
#pragma parameter response_time "LCD Response Time" 0.20 0.0 0.777 0.111

// Set video scale when used in console-border shaders
#pragma parameter video_scale "Video Scale" 3.0 2.0 20.0 1.0

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

////////////////////////////////////////////////////////////////////////////////
// Vertex shader                                                              //
////////////////////////////////////////////////////////////////////////////////

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 2) out vec2 dot_size;
layout(location = 3) out vec2 one_texel;

////////////////////////////////////////////////////////////////////////////////
// Vertex definitions                                                         //
////////////////////////////////////////////////////////////////////////////////


// Largest integer scale of input video that will fit in the current output (y axis would typically be limiting on widescreens)
//#define video_scale         floor(registers.OutputSize.y * registers.SourceSize.w) <- moved to parameter

// Size of the scaled video
//#define scaled_video_out    (registers.SourceSize.xy * vec2(video_scale)) <- moved to parameter             

void main()
{
	vec2 scaled_video_out = (registers.SourceSize.xy * vec2(registers.video_scale));
    // Remaps position to integer scaled output
    gl_Position = global.MVP * Position / vec4( vec2(registers.OutputSize.xy / scaled_video_out), 1.0, 1.0 );   
    vTexCoord = TexCoord;
    dot_size = registers.SourceSize.zw;
    one_texel = 1.0 / (registers.SourceSize.xy * registers.video_scale);
}

////////////////////////////////////////////////////////////////////////////////
// Fragment shader                                                            //
////////////////////////////////////////////////////////////////////////////////

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 2) in vec2 dot_size;
layout(location = 3) in vec2 one_texel;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D OriginalHistory2;
layout(set = 0, binding = 5) uniform sampler2D OriginalHistory3;
layout(set = 0, binding = 6) uniform sampler2D OriginalHistory4;
layout(set = 0, binding = 7) uniform sampler2D OriginalHistory5;
layout(set = 0, binding = 8) uniform sampler2D OriginalHistory6;
layout(set = 0, binding = 9) uniform sampler2D OriginalHistory7;
layout(set = 0, binding = 10) uniform sampler2D COLOR_PALETTE;

////////////////////////////////////////////////////////////////////////////////
//fragment definitions                                                        //
////////////////////////////////////////////////////////////////////////////////

#define foreground_color texture(COLOR_PALETTE, vec2(0.75, 0.5)).rgb                 //hardcoded to look up the foreground color from the right half of the palette image
//#define rgb_to_alpha(rgb) ( ((rgb.r + rgb.g + rgb.b) / 3.0) + (is_on_dot * vec2(registers.baseline_alpha), 1.0) )       //averages rgb values (allows it to work with color games), modified for contrast and base alpha


// Frame sampling definitions
#define curr_rgb  abs(1.0 - texture(Source,           vTexCoord).rgb)
#define prev0_rgb abs(1.0 - texture(OriginalHistory1, vTexCoord).rgb)
#define prev1_rgb abs(1.0 - texture(OriginalHistory2, vTexCoord).rgb)
#define prev2_rgb abs(1.0 - texture(OriginalHistory3, vTexCoord).rgb)
#define prev3_rgb abs(1.0 - texture(OriginalHistory4, vTexCoord).rgb)
#define prev4_rgb abs(1.0 - texture(OriginalHistory5, vTexCoord).rgb)
#define prev5_rgb abs(1.0 - texture(OriginalHistory6, vTexCoord).rgb)
#define prev6_rgb abs(1.0 - texture(OriginalHistory7, vTexCoord).rgb)

void main()
{
    // Determine if the corrent texel lies on a dot or in the space between dots
    float is_on_dot = 0.0;                   
    if ( mod(vTexCoord.x, dot_size.x) > one_texel.x && mod(vTexCoord.y, dot_size.y * 1.0001) > one_texel.y )
        is_on_dot = 1.0;

    // Sample color from the current and previous frames, apply response time modifier
    // Response time effect implmented through an exponential dropoff algorithm
    vec3 input_rgb = curr_rgb;
    input_rgb += (prev0_rgb - input_rgb) * registers.response_time;
    input_rgb += (prev1_rgb - input_rgb) * pow(registers.response_time, 2.0);
    input_rgb += (prev2_rgb - input_rgb) * pow(registers.response_time, 3.0);
    input_rgb += (prev3_rgb - input_rgb) * pow(registers.response_time, 4.0);
    input_rgb += (prev4_rgb - input_rgb) * pow(registers.response_time, 5.0);
    input_rgb += (prev5_rgb - input_rgb) * pow(registers.response_time, 6.0);
    input_rgb += (prev6_rgb - input_rgb) * pow(registers.response_time, 7.0);

    float rgb_to_alpha = (input_rgb.r + input_rgb.g + input_rgb.b) / registers.grey_balance
                        + (is_on_dot * registers.baseline_alpha);

    // Apply foreground color and assign alpha value
    // Apply the foreground color to all texels -
    // the color will be modified by alpha later - and assign alpha based on rgb input
    vec4 out_color = vec4(foreground_color, rgb_to_alpha);  

    // Overlay the matrix
    // If the fragment is not on a dot, set its alpha value to 0
    out_color.a *= is_on_dot;

    FragColor = out_color;
}

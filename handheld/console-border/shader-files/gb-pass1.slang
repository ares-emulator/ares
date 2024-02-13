#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	float blending_mode;
    float adjacent_texel_alpha_blending;
    float video_scale;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

////////////////////////////////////////////////////////////////////////////////
// Config                                                                     //
////////////////////////////////////////////////////////////////////////////////

// 0 - only the space between dots is blending
// 1 - all texels are blended
#pragma parameter blending_mode "Blending Mode" 0.0 0.0 1.0 1.0

// The amount of alpha swapped between neighboring texels
#pragma parameter adjacent_texel_alpha_blending "Neighbor Blending" 0.76 0.0 1.0 0.01

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
layout(location = 1) out vec2 texel;
layout(location = 2) out vec2 blur_coords_up;
layout(location = 3) out vec2 blur_coords_down;
layout(location = 4) out vec2 blur_coords_right;
layout(location = 5) out vec2 blur_coords_left;
layout(location = 6) out vec2 blur_coords_upright;
layout(location = 7) out vec2 blur_coords_upleft;
layout(location = 8) out vec2 blur_coords_downright;
layout(location = 9) out vec2 blur_coords_downleft;
layout(location = 10) out vec2 blur_coords_lower_bound;
layout(location = 11) out vec2 blur_coords_upper_bound;
layout(location = 12) out vec2 lcd_bounds_upleft;
layout(location = 13) out vec2 lcd_bounds_downright;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;

    texel = registers.SourceSize.zw;
    blur_coords_down      = vTexCoord + vec2(0.0, texel.y);
    blur_coords_up        = vTexCoord + vec2(0.0, -texel.y);
    blur_coords_right     = vTexCoord + vec2(texel.x,  0.0);
    blur_coords_left      = vTexCoord + vec2(-texel.x, 0.0);
    blur_coords_downright = vTexCoord + vec2(texel.x, texel.y);
    blur_coords_downleft  = vTexCoord + vec2(-texel.x, texel.y);
    blur_coords_upright   = vTexCoord + vec2(texel.x, -texel.y);
    blur_coords_upleft    = vTexCoord + vec2(-texel.x, -texel.y);

    blur_coords_lower_bound = vec2(0.0);
    blur_coords_upper_bound = texel * (registers.OutputSize.xy - vec2(2.0));

    vec2 half_lcd_size = vec2(0.5) * ((registers.OriginalSize.xy * registers.video_scale) / registers.OutputSize.xy);
    lcd_bounds_upleft = vec2(0.5) - half_lcd_size;
    lcd_bounds_downright = vec2(0.5) + half_lcd_size;
}

////////////////////////////////////////////////////////////////////////////////
// Fragment shader                                                            //
////////////////////////////////////////////////////////////////////////////////

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 texel;
layout(location = 2) in vec2 blur_coords_up;
layout(location = 3) in vec2 blur_coords_down;
layout(location = 4) in vec2 blur_coords_right;
layout(location = 5) in vec2 blur_coords_left;
layout(location = 6) in vec2 blur_coords_upright;
layout(location = 7) in vec2 blur_coords_upleft;
layout(location = 8) in vec2 blur_coords_downright;
layout(location = 9) in vec2 blur_coords_downleft;
layout(location = 10) in vec2 blur_coords_lower_bound;
layout(location = 11) in vec2 blur_coords_upper_bound;
layout(location = 12) in vec2 lcd_bounds_upleft;
layout(location = 13) in vec2 lcd_bounds_downright;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

////////////////////////////////////////////////////////////////////////////////
// Fragment definitions                                                       //
////////////////////////////////////////////////////////////////////////////////

// Clamp the blur coords to the input texture size so it doesn't attempt to sample off the texture (it'll retrieve float4(0.0) and darken the edges otherwise)
// Then return its alpha
float clamped_coord_alpha(vec2 blur_coords)
{
    vec2 clamped_coord = clamp(blur_coords, blur_coords_lower_bound, blur_coords_upper_bound);
    return texture(Source, clamped_coord).a;
}

// Used for gap direction detection
bool is_gap(vec2 coords_side_1, vec2 coords_side_2)
{
    float alpha_side_1 = texture(Source, coords_side_1).a;
    float alpha_side_2 = texture(Source, coords_side_2).a;

    return alpha_side_1 == 0.0 && alpha_side_2 == 0.0;
}

// A simple blur technique that softens harsh color transitions
// Specialized to only blur alpha values
// Blends only the texels between dots
vec4 blend_gap_mode(vec4 out_color)
{
    if(out_color.a == 0.0)
    {
        // Check if our gap is horizontal or vertical
        bool is_gap_horizontal = is_gap(blur_coords_left, blur_coords_right);
        bool is_gap_vertical = is_gap(blur_coords_up, blur_coords_down);

        // When in a gap interesection, average the 4 diagonal dots
        if(is_gap_horizontal && is_gap_vertical)
        {
            // Set the current fragment alpha value to the average of alpha differences between neighboring texels
            out_color.a =
            (
                (clamped_coord_alpha(blur_coords_upleft) - out_color.a) +
                (clamped_coord_alpha(blur_coords_upright) - out_color.a) +
                (clamped_coord_alpha(blur_coords_downleft) - out_color.a) +
                (clamped_coord_alpha(blur_coords_downright) - out_color.a)
            ) / 4;
        }
        // When in an horizontal gap, average the above and bellow texels
        else if(is_gap_horizontal)
        {
            out_color.a =
            (
                (clamped_coord_alpha(blur_coords_up) - out_color.a) +
                (clamped_coord_alpha(blur_coords_down) - out_color.a)
            ) / 2;
        }
        // When in a vertical gap, average the left and right texels
        else if(is_gap_vertical)
        {
            out_color.a =
            (
                (clamped_coord_alpha(blur_coords_right) - out_color.a) +
                (clamped_coord_alpha(blur_coords_left) - out_color.a)
            ) / 2;
        }

        // Modify the alpha value based on how high the "Neighbor Blending" parameter is set to
        out_color.a *= registers.adjacent_texel_alpha_blending;
    }

    return out_color;
}

// Blends all texels
vec4 blend_all_mode(vec4 out_color)
{
    // Average the alpha differences between neighboring texels, apply the parameter modifier, then sum the result to the current fragment alpha value
    out_color.a +=
    (
        (clamped_coord_alpha(blur_coords_up) - out_color.a) +
        (clamped_coord_alpha(blur_coords_down) - out_color.a) +
        (clamped_coord_alpha(blur_coords_right) - out_color.a) +
        (clamped_coord_alpha(blur_coords_left) - out_color.a)
    ) / 4 * registers.adjacent_texel_alpha_blending;

    return out_color;
}

void main()
{
    vec4 out_color = texture(Source, vTexCoord).rgba;
    vec4 blended;

    if(registers.blending_mode == 1.0)
        blended = blend_all_mode(out_color);
    else
        blended = blend_gap_mode(out_color);

    if ((vTexCoord.x < lcd_bounds_upleft.x) || (vTexCoord.x > lcd_bounds_downright.x) || (vTexCoord.y < lcd_bounds_upleft.y) || (vTexCoord.y > lcd_bounds_downright.y))
    {
        blended = vec4(0.0);
    }

    FragColor = blended;
}

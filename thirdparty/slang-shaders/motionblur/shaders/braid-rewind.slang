#version 450
/*
    Braid Rewind
    Authors: hunterk, cgwg
 
    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.
*/

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    int FrameDirection;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
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
    vec4 current = texture(Source, vTexCoord);

    vec4 color = 
        texture(OriginalHistory7, vTexCoord) +
        texture(OriginalHistory6, vTexCoord) +
        texture(OriginalHistory5, vTexCoord) +
        texture(OriginalHistory4, vTexCoord) +
        texture(OriginalHistory3, vTexCoord) +
        texture(OriginalHistory2, vTexCoord) +
        texture(OriginalHistory1, vTexCoord) +
        current;

    vec4 sepia = vec4(1.0, 0.8, 0.6, 1.0); 

    if (global.FrameDirection < 0)
    {
        current = ((current + (color * 0.142857142857143)) * 0.5) * sepia;
    }

    FragColor = current;
}

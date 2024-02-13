#version 450
/* 
 * gizmo98 blue noise dithering shader
 * Copyright (C) 2023 gizmo98
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 * version 0.1, 16.05.2023
 * ---------------------------------------------------------------------------------------
 * - initial commit
 * 
 * https://github.com/gizmo98/gizmo-crt-shader
 *
 * uses parts of texture anti-aliasing shader from Ikaros https://www.shadertoy.com/view/ldsSRX
 * uses blue noise texture from http://momentsingraphics.de/BlueNoise.html
 */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float COLOR_DEPTH;
	float DITHER_TUNE;
	float MONOCHROME;
	float EGA_PALETTE;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter COLOR_DEPTH "Color depth in Bits"            1.0 1.0 8.0 1.0
#pragma parameter DITHER_TUNE "Tune dithering"                 0.0 -64.0 64.0 1.0
#pragma parameter MONOCHROME "Monochrome"                      0.0 0.0 1.0 1.0
#pragma parameter EGA_PALETTE "EGA palette"                    0.0 0.0 1.0 1.0

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D BlueNoiseTexture0;

vec4 DitherPattern(vec4 col, vec2 coord)
{
    float count = floor(fract(params.FrameCount / 11.0) * 11.0);
    vec2 texSize = vec2(textureSize(BlueNoiseTexture0,0));
    
    coord = fract(coord/ texSize) * texSize;
    vec4 color;

    if (count == 0.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(coord),0);
    else if (count == 1.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(coord.y,coord.x),0).brga;
    else if (count == 2.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(coord),0).gbra;    
    else if (count == 3.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(coord.y,coord.x),0).rbga;
    else if (count == 4.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(coord),0).bgra; 
    else if (count == 5.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(coord.y,coord.x),0).grba;
    else if (count == 6.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(texSize - coord),0);
    else if (count == 7.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(texSize.y - coord.y,texSize.x - coord.x),0).brga;
    else if (count == 8.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(texSize - coord),0).gbra;    
    else if (count == 9.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(texSize.y - coord.y,texSize.x - coord.x),0).rbga;
    else if (count == 10.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(texSize - coord),0).bgra; 
    else if (count == 11.0)
        color = texelFetch(BlueNoiseTexture0,ivec2(texSize.y - coord.y,texSize.x - coord.x),0).grba;

    if (params.MONOCHROME == 1.0)
        color.rgb = color.rrr;
    
    vec3 threshold = color.rgb;
    float multiplier = pow(2.0,8.0 - params.COLOR_DEPTH) - 1.0;
    
    threshold = threshold - 0.5;
    threshold *= ((multiplier + params.DITHER_TUNE) / 255.0);
        
    col.rgb += threshold; 				           
    return col;
}

vec2 saturateA(in vec2 x)
{
    return clamp(x, 0.0, 1.0);
}

vec2 magnify(in vec2 uv, in vec2 res)
{
    uv *= res; 
    return (saturateA(fract(uv) / saturateA(fwidth(uv))) + floor(uv) - 0.5) / res.xy;
}

vec4 textureAA(in vec2 uv){
    vec2 texSize = textureSize(Source,0);
    vec2 st = uv*texSize.xy;

    vec2 ist = floor(st);
    vec4 col = texture(Source, uv);

    if (params.MONOCHROME == 1.0)
        col.rgb = vec3(col.r + col.g + col.b) / 3.0;
    
    col = DitherPattern(col , ist + 0.5);
    return col;
}

vec4 ColorDepthReduction(vec4 col)
{
    float divider = pow(2.0,params.COLOR_DEPTH) - 1.0; 
    col.rgb *= divider;
    col.rgb = floor(col.rgb) + step(0.5, fract(col.rgb));
    col.rgb /= divider;
    return col;
}

vec4 EGAPalette(vec4 col)
{    
    float divider = 3.0;
    vec3 c = floor(col.rgb * divider) + step(0.5, fract(col.rgb * divider));
    if (c.rgb == vec3(0.0,0.0,0.0) ||
        c.rgb == vec3(1.0,1.0,1.0) || 
        c.rgb == vec3(2.0,2.0,2.0) ||
        c.rgb == vec3(3.0,3.0,3.0) ||
        c.rgb == vec3(2.0,1.0,0.0))
        col.rgb = col.rgb;
    // bright green
    else if (c.rgb == vec3(0.0,3.0,0.0)||
             c.rgb == vec3(0.0,3.0,2.0)||
             c.rgb == vec3(2.0,3.0,0.0)||
             c.rgb == vec3(0.0,3.0,1.0)||
             c.rgb == vec3(1.0,3.0,1.0)||
             c.rgb == vec3(1.0,3.0,0.0)||
             c.rgb == vec3(2.0,3.0,1.0)||
             c.rgb == vec3(2.0,3.0,2.0))
        col.rgb = vec3(1.0,3.0,1.0) / divider;
    // green  
    else if (c.rgb == vec3(0.0,2.0,0.0)||
             c.rgb == vec3(0.0,2.0,1.0)||
             c.rgb == vec3(0.0,1.0,0.0)||
             c.rgb == vec3(0.0,1.0,1.0)||
             c.rgb == vec3(1.0,2.0,1.0)||
             c.rgb == vec3(1.0,2.0,0.0)||
             c.rgb == vec3(0.0,1.0,1.0))
        col.rgb = vec3(0.0,2.0,0.0) / divider; 
    // bright red
    else if (c.rgb == vec3(3.0,0.0,0.0)||
             c.rgb == vec3(3.0,0.0,1.0)||
             c.rgb == vec3(3.0,1.0,0.0)||
             c.rgb == vec3(3.0,1.0,1.0))
        col.rgb = vec3(3.0,1.0,1.0) / divider;
    // red  
    else if (c.rgb == vec3(2.0,0.0,0.0)||
             c.rgb == vec3(2.0,0.0,1.0)||
             c.rgb == vec3(1.0,0.0,0.0))
        col.rgb = vec3(2.0,0.0,0.0) / divider; 
    // bright cyan
    else if (c.rgb == vec3(0.0,3.0,3.0)||
             c.rgb == vec3(1.0,3.0,3.0)||
             c.rgb == vec3(2.0,3.0,3.0))
        col.rgb = vec3(1.0,3.0,3.0) / divider;
    // cyan
    else if (c.rgb == vec3(0.0,2.0,2.0)||
             c.rgb == vec3(1.0,2.0,2.0))
        col.rgb = vec3(0.0,2.0,2.0) / divider;
    // bright blue
    else if (c.rgb == vec3(0.0,2.0,3.0)||
             c.rgb == vec3(1.0,2.0,3.0)||
             c.rgb == vec3(0.0,0.0,3.0))
        col.rgb = vec3(1.0,1.0,3.0) / divider;
    // blue  
    else if (c.rgb == vec3(0.0,0.0,2.0)||
             c.rgb == vec3(0.0,0.0,1.0)||
             c.rgb == vec3(0.0,1.0,2.0)||
             c.rgb == vec3(1.0,1.0,3.0)||
             c.rgb == vec3(0.0,1.0,3.0))
        col.rgb = vec3(0.0,0.0,2.0) / divider; 
    // brown  
    else if (c.rgb == vec3(2.0,1.0,0.0)||
             c.rgb == vec3(2.0,1.0,1.0)||
             c.rgb == vec3(1.0,1.0,0.0))
        col.rgb = vec3(2.0,1.0,0.0) / divider; 
    // bright yellow  
    else if (c.rgb == vec3(3.0,3.0,0.0)||
             c.rgb == vec3(3.0,3.0,1.0)||
             c.rgb == vec3(3.0,3.0,2.0)||
             c.rgb == vec3(2.0,2.0,0.0)||
             c.rgb == vec3(2.0,2.0,1.0))
        col.rgb = vec3(3.0,3.0,1.0) / divider; 
    // magenta  
    else if (c.rgb == vec3(2.0,0.0,2.0)||
             c.rgb == vec3(2.0,0.0,3.0)||
             c.rgb == vec3(2.0,1.0,2.0)||
             c.rgb == vec3(2.0,1.0,3.0))
        col.rgb = vec3(2.0,0.0,2.0) / divider;     
    // bright magenta  
    else if (c.rgb == vec3(3.0,0.0,2.0)||
             c.rgb == vec3(3.0,0.0,3.0)||
             c.rgb == vec3(3.0,2.0,3.0)||
             c.rgb == vec3(3.0,1.0,3.0)||
             c.rgb == vec3(3.0,1.0,2.0))
        col.rgb = vec3(3.0,1.0,3.0) / divider; 
    else if (c.r == 0.0)
        col.gb = step(2.0,c.gb) * 2.0 / divider;
    else if (c.g == 0.0)
        col.rb = step(2.0,c.rb) * 2.0 / divider;
    else if (c.b == 0.0)
        col.rg = step(2.0,c.rg) * 2.0 / divider;
    else if (c.r == 3.0)
        col.gb = step(1.0,c.gb) / divider;
    else if (c.g == 3.0)
        col.rb = step(1.0,c.rb) / divider;
    else if (c.b == 3.0)
        col.rg = step(1.0,c.rg) / divider;
    return col;
}

void main()
{
    vec2 texcoord = vTexCoord.xy;
    FragColor = textureAA(texcoord);
    FragColor = ColorDepthReduction(FragColor);
    if (params.EGA_PALETTE == 1.0)
        FragColor = EGAPalette(FragColor);
}

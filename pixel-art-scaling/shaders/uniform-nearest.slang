#version 450
/* 
 * gizmo98 uniform-nearest shader
 * Copyright (C) 2023 gizmo98
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 * version 0.1, 28.04.2023
 * ---------------------------------------------------------------------------------------
 * - initial commit
 * 
 * https://github.com/gizmo98/gizmo-crt-shader
 *
 * This shader texture AA shader code and subpixel scaling to produce a nearest neighbor
 * look with even placed pixels. If nearest neighbor is applied to fractional scaled output 
 * pixels look uneven and moving sprites change dimension which looks ugly.
 *
 * BGR_LCD_PATTERN most LCDs have a RGB pixel pattern. Enable BGR pattern with this switch
 *
 * uses parts of texture anti-aliasing shader from Ikaros https://www.shadertoy.com/view/ldsSRX
 */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float BGR_LCD_PATTERN;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter BGR_LCD_PATTERN "BGR output pattern"         0.0 0.0 1.0 1.0

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
    vec2 texSize = vec2(textureSize(Source, 0));
    uv = magnify(uv,texSize.xy);
    uv = uv*texSize.xy + 0.5;

    vec2 iuv = floor(uv);
    vec2 fuv = uv - iuv;    
    uv = vec2(uv + vec2(-0.5,-0.5)) / texSize.xy;
    return texture( Source, uv );
}

vec4 textureSubpixelScaling(in vec2 uvr, in vec2 uvg, in vec2 uvb ){
    return vec4(textureAA(uvr).r, textureAA(uvg).g, textureAA(uvb).b, 1.0);
}

float GetFuv(in vec2 uv){
    vec2 texSize = vec2(textureSize(Source, 0));
    uv = uv*texSize.xy + 0.5;
    vec2 iuv = floor(uv);
    vec2 fuv = uv - iuv;
    return abs((fuv*fuv*fuv*(fuv*(fuv*6.0-15.0)+10.0)).y - 0.5);
}

vec3 XCoords(in float coord, in float factor){
    float iGlobalTime = float(params.FrameCount)*0.025;
    float spread = 0.333;
    vec3 coords = vec3(coord);
    if(params.BGR_LCD_PATTERN == 1.0)
        coords.r += spread * 2.0;
    else
        coords.b += spread * 2.0;
    coords.g += spread;
    coords *= factor;
    return coords;
}

float YCoord(in float coord, in float factor){
    return coord * factor;
}

void main()
{
    vec2 texSize = vec2(textureSize(Source, 0));
    vec2 texcoord = vTexCoord.xy;

    vec2 fragCoord = texcoord.xy * params.OutputSize.xy;

    vec2 factor = texSize.xy / params.OutputSize.xy ;
    highp float yCoord = YCoord(fragCoord.y, factor.y) ;
    highp vec3 xCoords = XCoords(fragCoord.x, factor.x) ;

    vec2 coord_r = vec2(xCoords.r/ texSize.x, texcoord.y) ;
    vec2 coord_g = vec2(xCoords.g, yCoord) / texSize.xy;
    vec2 coord_b = vec2(xCoords.b, yCoord) / texSize.xy;

    FragColor = textureSubpixelScaling(coord_r,coord_g,coord_b);
}

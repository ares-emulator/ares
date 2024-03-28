#version 450

// newpixie CRT
// by Mattias Gustavsson
// adapted for slang by hunterk

/*
------------------------------------------------------------------------------
This software is available under 2 licenses - you may choose the one you like.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2016 Mattias Gustavsson
Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OutputSize;
	float curvature;
	float vignette;
} params;

#pragma parameter curvature "Curvature" 2.0 0.0001 4.0 0.25
#pragma parameter vignette "Vignette" 1.0 0.0 1.0 0.05

#define gl_FragCoord (vTexCoord.xy * params.OutputSize.xy)

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   vec4 modpos = vec4(Position.x, 1.-Position.y, Position.z, Position.w);
   gl_Position = global.MVP * modpos;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 3) uniform sampler2D Source;

vec3 tsample( sampler2D samp, vec2 tc, float offs, vec2 resolution )
    {
    tc = tc * vec2(1.025, 0.92) + vec2(-0.0125, 0.04);
    vec3 s = pow( abs( texture( samp, vec2( tc.x, 1.0-tc.y ) ).rgb), vec3( 2.2 ) );
    return s*vec3(1.25);
    }
		
vec3 filmic( vec3 LinearColor )
    {
    vec3 x = max( vec3(0.0), LinearColor-vec3(0.004));
    return (x*(6.2*x+0.5))/(x*(6.2*x+1.7)+0.06);
    }
		
vec2 curve( vec2 uv )
    {
    uv = (uv - 0.5);// * 2.0;
//    uv.x *= 0.75;
    uv *= vec2(0.925, 1.095);  
   uv *= params.curvature;
    uv.x *= 1.0 + pow((abs(uv.y) / 4.0), 2.0);
    uv.y *= 1.0 + pow((abs(uv.x) / 3.0), 2.0);
    uv /= params.curvature;
    uv  += 0.5;
    uv =  uv *0.92 + 0.04;
    return uv;
    }
		
float rand(vec2 co)
    {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
    }
    
#define resolution params.OutputSize.xy

void main()
{
    vec2 uv = vTexCoord.xy;
    /* Curve */
    vec2 curved_uv = mix( curve( uv ), uv, 0.4 );
    float scale = -0.101;
    vec2 scuv = curved_uv*(1.0-scale)+scale/2.0+vec2(0.003, -0.001);
    
    uv = scuv;
		
    /* Main color, Bleed */
    vec3 col;

    float o =sin(gl_FragCoord.y*1.5)/resolution.x;
    float x = o*0.25;

    col.r = tsample(Source,vec2(x+scuv.x+0.0009,scuv.y+0.0009),resolution.y/800.0, resolution ).x+0.02;
    col.g = tsample(Source,vec2(x+scuv.x+0.0000,scuv.y-0.0011),resolution.y/800.0, resolution ).y+0.02;
    col.b = tsample(Source,vec2(x+scuv.x-0.0015,scuv.y+0.0000),resolution.y/800.0, resolution ).z+0.02;

    /* Level adjustment (curves) */
    col = clamp(col + col*col + col*col*col*col*col,vec3(0.0),vec3(10.0));

    /* Vignette. Increase the 4.0 factor for greater burnout effect in the center of the screen. */
    float vig = ((1.0-0.99*params.vignette) + 1.0*4.0*curved_uv.x*curved_uv.y*(1.0-curved_uv.x)*(1.0-curved_uv.y));
    vig = 1.3*pow(vig,0.5);
    col *= vig;
    /* Compensate the lack of vignette, in case we decide to comment it out for performance reasons */
    //col *= 0.6;

    /* Scanlines */
    float scans = clamp( 0.35+0.18*sin(curved_uv.y*resolution.y*1.5), 0.0, 1.0);
    float s = pow(scans,0.9);
    col = col * vec3(s);
		
    /* Vertical lines (shadow mask) */
    col*=1.0-0.23*(clamp((mod(gl_FragCoord.xy.x, 3.0))/2.0,0.0,1.0));
		
    /* Tone map */
    col = filmic( col );
		
    uv = curved_uv;

    FragColor = vec4( col, 1.0 );
}

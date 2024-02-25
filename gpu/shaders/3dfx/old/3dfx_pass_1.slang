#version 450

// "LeiFX" shader - Gamma process
// 
// 	Copyright (C) 2013-2014 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define saturate(c) clamp(c, 0.0, 1.0)
#define lerp(c) mix(c)
#define mul(a,b) (b*a)
#define fmod(c) mod(c)
#define frac(c) fract(c)
#define tex2D(c,d) texture(c,d)
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4
#define float2x2 mat2x2
#define float3x3 mat3x3
#define float4x4 mat4x4

float mod2(float x, float y)
{
	return x - y * floor (x/y);
}

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

void main()
{
	float3 outcolor = tex2D(Source, vTexCoord).rgb;
	
	float2 res;
	res.x = params.SourceSize.x;
	res.y = params.SourceSize.y;

	// Gamma scanlines
	// the Voodoo drivers usually supply a 1.3 gamma setting whether people liked it or not
	// but it was enough to brainwash the competition for looking 'too dark'

	float gammaed = 0.15;

	float leifx_linegamma = gammaed;
	float2 dithet = vTexCoord.xy * res.xy;
	dithet.y = vTexCoord.y * res.y;
	float horzline1 = 	(mod2(dithet.y, 	2.0));
	if (horzline1 < 1)	leifx_linegamma = 0;
	float leifx_gamma = 1.3 - gammaed + leifx_linegamma;


	outcolor.r = pow(outcolor.r, 1.0 / leifx_gamma);
	outcolor.g = pow(outcolor.g, 1.0 / leifx_gamma);
	outcolor.b = pow(outcolor.b, 1.0 / leifx_gamma);
	
   FragColor = vec4(outcolor, 1.0);
}
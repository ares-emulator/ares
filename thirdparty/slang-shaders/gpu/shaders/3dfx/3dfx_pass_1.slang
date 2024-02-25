#version 450

// "LeiFX" shader - Pixel filtering process
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

#define		FILTCAP		0.04	// filtered pixel should not exceed this 
#define		FILTCAPG	(FILTCAP / 2)
#define		LEIFX_PIXELWIDTH	0.50f

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
	
	float2 pixel;

	pixel.x = params.SourceSize.z;
	pixel.y = params.SourceSize.w;

	float3 pixel1 = tex2D(Source, vTexCoord + float2((pixel.x), 0)).rgb;
	float3 pixel2 = tex2D(Source, vTexCoord + float2(-pixel.x, 0)).rgb;
	float3 pixelblend;
	
// New filter
	{
		float3 pixeldiff;
		float3 pixelmake;		
		float3 pixeldiffleft;

		pixelmake.rgb = float3(0.0);
		pixeldiff.rgb = pixel2.rgb- outcolor.rgb;

		pixeldiffleft.rgb = pixel1.rgb - outcolor.rgb;

		if (pixeldiff.r > FILTCAP) 		pixeldiff.r = FILTCAP;
		if (pixeldiff.g > FILTCAPG) 		pixeldiff.g = FILTCAPG;
		if (pixeldiff.b > FILTCAP) 		pixeldiff.b = FILTCAP;

		if (pixeldiff.r < -FILTCAP) 		pixeldiff.r = -FILTCAP;
		if (pixeldiff.g < -FILTCAPG) 		pixeldiff.g = -FILTCAPG;
		if (pixeldiff.b < -FILTCAP) 		pixeldiff.b = -FILTCAP;

		if (pixeldiffleft.r > FILTCAP) 		pixeldiffleft.r = FILTCAP;
		if (pixeldiffleft.g > FILTCAPG) 	pixeldiffleft.g = FILTCAPG;
		if (pixeldiffleft.b > FILTCAP) 		pixeldiffleft.b = FILTCAP;

		if (pixeldiffleft.r < -FILTCAP) 	pixeldiffleft.r = -FILTCAP;
		if (pixeldiffleft.g < -FILTCAPG) 	pixeldiffleft.g = -FILTCAPG;
		if (pixeldiffleft.b < -FILTCAP) 	pixeldiffleft.b = -FILTCAP;

		pixelmake.rgb = (pixeldiff.rgb / 4) + (pixeldiffleft.rgb / 16);
		outcolor.rgb = (outcolor.rgb + pixelmake.rgb);
	}	
	
   FragColor = vec4(outcolor, 1.0);
}
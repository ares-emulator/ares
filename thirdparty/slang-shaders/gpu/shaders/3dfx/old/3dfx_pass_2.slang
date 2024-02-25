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
	float LEIFX_BLURFACTOR;
} params;

#pragma parameter LEIFX_BLURFACTOR "LeiFX Blur Factor" 0.69 0.00 1.00 0.01

#define LEIFX_BLURFACTOR params.LEIFX_BLURFACTOR

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

	// Sample things.

	float blendy;	// to blend unblended with blend... trying to smooth the jag :(
	float blenda;	

	float blendfactor;

	float3 pixel1 = tex2D(Source, vTexCoord + float2((pixel.x * 0.15), 0)).rgb;
	float3 pixel2 = tex2D(Source, vTexCoord + float2(-pixel.x * 0.22, 0)).rgb;
	float3 pixel0 = tex2D(Source, vTexCoord + float2(0, 0)).rgb;

	float3 pixelblend;


	float gary1 = dot(pixel1.rgb,float3(1.0));
	float gary2 = dot(pixel2.rgb,float3(1.0));

	float mean = 1.0;
	mean = gary1 - gary2;

	if (mean < 0)	mean *= -1;
	if (mean > 1) mean = 1;	
	mean = pow(mean, LEIFX_BLURFACTOR);	

	if (mean > 1)
      mean = 1;	

	{
		// variably BLEND IT ALL TO H*CK!!!!
		blendy = 1 - mean;
		blenda = 1 - blendy;
		pixel0 /= 3;
		pixel1 /= 3;
		pixel2 /= 3;
   		pixelblend.rgb = pixel0 + pixel1 + pixel2;
		outcolor.rgb = (pixelblend.rgb * blendy) + (outcolor.rgb * blenda);
	}
	
   FragColor = vec4(outcolor, 1.0);
}
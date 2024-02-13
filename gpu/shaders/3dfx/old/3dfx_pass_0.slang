#version 450

// "LeiFX" shader - "dither" and reduction process
// 
// 	Copyright (C) 2013-2014 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.

// This table came from the wikipedia article about Ordered Dithering. NOT MAME.  Just to clarify.
float erroredtable[16] = {
	16,4,13,1,   
	8,12,5,9,
	14,2,15,3,
	6,10,7,11		
};

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

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

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define DITHERAMOUNT		0.5f // was 0.33f
#define DITHERBIAS		-1  // 0 to 16, biases the value of the dither up.  - was 8

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
	float2 res;
	float3 outcolor = tex2D(Source, vTexCoord).rgb;
	res.x = params.SourceSize.x;
	res.y = params.SourceSize.y;
	
	float2 ditheu = vTexCoord.xy * res.xy;

	ditheu.x = vTexCoord.x * res.x;
	ditheu.y = vTexCoord.y * res.y;

	// Dither. Total rewrite.
	// NOW, WHAT PIXEL AM I!??

	int ditdex = 	int(mod(ditheu.x, 4.0)) * 4 + int(mod(ditheu.y, 4.0)); // 4x4!
	vec3 color;
	vec3 colord;
	color.r = outcolor.r * 255;
	color.g = outcolor.g * 255;
	color.b = outcolor.b * 255;
	float yeh = 0.0;
	float ohyes = 0.0;
	
//	for (yeh=ditdex; yeh<(ditdex+16); yeh++) ohyes = pow(erroredtable[yeh-15], 0.72f);
	if (yeh++==ditdex) ohyes = erroredtable[0];
	else if (yeh++==ditdex) ohyes = erroredtable[1];
	else if (yeh++==ditdex) ohyes = erroredtable[2];
	else if (yeh++==ditdex) ohyes = erroredtable[3];
	else if (yeh++==ditdex) ohyes = erroredtable[4];
	else if (yeh++==ditdex) ohyes = erroredtable[5];
	else if (yeh++==ditdex) ohyes = erroredtable[6];
	else if (yeh++==ditdex) ohyes = erroredtable[7];
	else if (yeh++==ditdex) ohyes = erroredtable[8];
	else if (yeh++==ditdex) ohyes = erroredtable[9];
	else if (yeh++==ditdex) ohyes = erroredtable[10];
	else if (yeh++==ditdex) ohyes = erroredtable[11];
	else if (yeh++==ditdex) ohyes = erroredtable[12];
	else if (yeh++==ditdex) ohyes = erroredtable[13];
	else if (yeh++==ditdex) ohyes = erroredtable[14];
	else if (yeh++==ditdex) ohyes = erroredtable[15];

	colord.r = color.r + ohyes;
	colord.g = color.g + (ohyes / 2);
	colord.b = color.b + ohyes;
	outcolor.rgb = colord.rgb * 0.003921568627451; // divide by 255, i don't trust em
	
	//
	// Reduce to 16-bit color
	//

	float3 why = float3(1.0);
	float3 reduceme = float3(1.0);
	float radooct = 32;	// 32 is usually the proper value

	reduceme.r = pow(outcolor.r, why.r);  
	reduceme.r *= radooct;	
	reduceme.r = int(floor(reduceme.r));	
	reduceme.r /= radooct; 
	reduceme.r = pow(reduceme.r, why.r);

	reduceme.g = pow(outcolor.g, why.g);  
	reduceme.g *= radooct * 2;	
	reduceme.g = int(floor(reduceme.g));	
	reduceme.g /= radooct * 2; 
	reduceme.g = pow(reduceme.g, why.g);

	reduceme.b = pow(outcolor.b, why.b);  
	reduceme.b *= radooct;	
	reduceme.b = int(floor(reduceme.b));	
	reduceme.b /= radooct; 
	reduceme.b = pow(reduceme.b, why.b);

	outcolor.rgb = reduceme.rgb;

   FragColor = vec4(outcolor, 1.0);
}
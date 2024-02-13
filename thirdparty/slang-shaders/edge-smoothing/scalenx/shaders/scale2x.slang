#version 450

/*
	Scale2x

Filter:	Nearest
Scale:	2x

Scale2x is real-time graphics effect able to increase the size of small bitmaps guessing the missing pixels without blurring the images.
It was originally developed for the AdvanceMAME project in the year 2001 to improve the quality of old games with a low video resolution.

Homepage: http://scale2x.sourceforge.net/
Copyright (C) 2001, 2002, 2003, 2004 Andrea Mazzoleni 
License: GNU-GPL  

*/

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
#define lerp(a,b,c) mix(a,b,c)
#define mul(a,b) (b*a)
#define fmod(c,d) mod(c,d)
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

#define decal Source

bool eq(float3 A, float3 B){
	return (A==B);
}

bool neq(float3 A, float3 B){
	return (A!=B);
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;

void main()
{
   gl_Position = global.MVP * Position;
   texCoord = TexCoord; // E
   
   	float2 ps = float2(params.SourceSize.z, params.SourceSize.w);
	float dx = ps.x;
	float dy = ps.y;

	t1 = TexCoord.xyxy + float4(  0,-dy,-dx,  0);	// B, D
	t2 = TexCoord.xyxy + float4( dx,  0,  0, dy);	// F, H	
}

#pragma stage fragment
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	// subpixel determination
	float2 fp = floor(2.0 * frac(texCoord*params.SourceSize.xy));

	/*
		  B		E0 E1
		D E F		E2 E3
		  H
	*/

	// reading the texels
	float3 B = tex2D(decal, t1.xy).xyz;
	float3 D = tex2D(decal, t1.zw).xyz;
	float3 E = tex2D(decal, texCoord).xyz;
	float3 F = tex2D(decal, t2.xy).xyz;
	float3 H = tex2D(decal, t2.zw).xyz;

	// rules
	float3 E0 = eq(B,D) ? B : E;
	float3 E1 = eq(B,F) ? B : E;
	float3 E2 = eq(H,D) ? H : E;
	float3 E3 = eq(H,F) ? H : E;

	// general condition & subpixel output
	FragColor = vec4(neq(B,H) && neq(D,F) ? (fp.y == 0 ? (fp.x == 0 ? E0 : E1) : (fp.x == 0 ? E2 : E3)) : E, 1.0);
}
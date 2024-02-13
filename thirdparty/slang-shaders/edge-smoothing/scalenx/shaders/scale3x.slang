#version 450

/*
	Scale3x

Filter:	Nearest
Scale:	3x

Scale3x is real-time graphics effect able to increase the size of small bitmaps guessing the missing pixels without blurring the images.
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
layout(location = 3) out vec4 t3;

void main()
{
   gl_Position = global.MVP * Position;
   texCoord = TexCoord;
   
   	float2 ps = float2(params.SourceSize.z, params.SourceSize.w);
	float dx = ps.x;
	float dy = ps.y;

	t1 = texCoord.xxxy + float4(-dx,  0, dx,-dy);	// A, B, C
	t2 = texCoord.xxxy + float4(-dx,  0, dx,  0);	// D, E, F
	t3 = texCoord.xxxy + float4(-dx,  0, dx, dy);	// G, H, I
}

#pragma stage fragment
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	// subpixel determination
	float2 fp = floor(3.0 * frac(texCoord*params.SourceSize.xy));

	/*
		A B C		E0 E1 E2
		D E F		E3 E4 E5
		G H I		E6 E7 E8
	*/

	// reading the texels
	float3 A = tex2D(decal, t1.xw).xyz;
	float3 B = tex2D(decal, t1.yw).xyz;
	float3 C = tex2D(decal, t1.zw).xyz;
	float3 D = tex2D(decal, t2.xw).xyz;
	float3 E = tex2D(decal, t2.yw).xyz;
	float3 F = tex2D(decal, t2.zw).xyz;
	float3 G = tex2D(decal, t3.xw).xyz;
	float3 H = tex2D(decal, t3.yw).xyz;
	float3 I = tex2D(decal, t3.zw).xyz;

	// equality checks
	bool eqBD = eq(B,D), eqBF = eq(B,F), eqHD = eq(H,D), eqHF = eq(H,F), neqEA = neq(E,A), neqEC = neq(E,C), neqEG = neq(E,G), neqEI = neq(E,I); 

	// rules
	float3 E0 = eqBD ? B : E;
	float3 E1 = eqBD && neqEC || eqBF && neqEA ? B : E;
	float3 E2 = eqBF ? B : E;
	float3 E3 = eqBD && neqEG || eqHD && neqEA ? D : E;
	float3 E5 = eqBF && neqEI || eqHF && neqEC ? F : E;
	float3 E6 = eqHD ? H : E;
	float3 E7 = eqHD && neqEI || eqHF && neqEG ? H : E;
	float3 E8 = eqHF ? H : E;

	// general condition & subpixel output
	FragColor = vec4(neq(B,H) && neq(D,F) ? (fp.y == 0 ? (fp.x == 0 ? E0 : fp.x == 1 ? E1 : E2) : (fp.y == 1 ? (fp.x == 0 ? E3 : fp.x == 1 ? E : E5) : (fp.x == 0 ? E6 : fp.x == 1 ? E7 : E8))) : E, 1.0);
}
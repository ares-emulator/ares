#version 450

/*
   Hyllian's Data Dependent Triangulation Shader
   
   Copyright (C) 2011-2016 Hyllian/Jararaca - sergiogdb@gmail.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
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

const float3 dtt = float3(65536,255,1);

float reduce(float3 color)
{
	return dot(color, dtt);
}

float3 bilinear(float p, float q, float3 A, float3 B, float3 C, float3 D)
{
	return ((1-p)*(1-q)*A + p*(1-q)*B + (1-p)*q*C + p*q*D);
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec2 loc;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
   
   float2 ps = float2(params.SourceSize.z, params.SourceSize.w);
	float dx = ps.x;
	float dy = ps.y;

	t1.xy = float2( dx,  0); // F
	t1.zw = float2(  0, dy); // H
	loc = vTexCoord*params.SourceSize.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec2 loc;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	float2 pos = frac(loc)-float2(0.5, 0.5); // pos = pixel position
	float2 dir = sign(pos); // dir = pixel direction

	float2 g1 = dir*t1.xy;
	float2 g2 = dir*t1.zw;

	float3 A = tex2D(decal, vTexCoord       ).xyz;
	float3 B = tex2D(decal, vTexCoord +g1   ).xyz;
	float3 C = tex2D(decal, vTexCoord    +g2).xyz;
	float3 D = tex2D(decal, vTexCoord +g1+g2).xyz;

	float a = reduce(A);
	float b = reduce(B);
	float c = reduce(C);
	float d = reduce(D);

	float p = abs(pos.x);
	float q = abs(pos.y);

	float k = distance(pos,g1 * params.SourceSize.xy);
	float l = distance(pos,g2 * params.SourceSize.xy);

	if (abs(a-d) < abs(b-c))
	{
		if (k < l)
		{
			C = A + D - B;
		}
		else if (k > l)
		{
			B = A + D - C;
		}
	}
	else if (abs(a-d) > abs(b-c))
	{
		D = B + C - A;
	}

	float3 color_old = bilinear(p, q, A, B, C, D);

	float2 texsize = params.SourceSize.xy;//IN.texture_size;
	const float scale_factor = 1.0;
	float2 delta = 0.5 / (texsize * scale_factor);
	float dx = delta.x;
	float dy = delta.y;

	float3 c00 = tex2D(decal, vTexCoord + float2(-dx, -dy)).xyz;
	float3 c01 = tex2D(decal, vTexCoord + float2(-dx, 0)).xyz;
	float3 c02 = tex2D(decal, vTexCoord + float2(-dx, dy)).xyz;
	float3 c10 = tex2D(decal, vTexCoord + float2(0, -dy)).xyz;
	float3 c11 = tex2D(decal, vTexCoord + float2(0, 0)).xyz;
	float3 c12 = tex2D(decal, vTexCoord + float2(0, dy)).xyz;
	float3 c20 = tex2D(decal, vTexCoord + float2(dx, -dy)).xyz;
	float3 c21 = tex2D(decal, vTexCoord + float2(dx, 0)).xyz;
	float3 c22 = tex2D(decal, vTexCoord + float2(dx, dy)).xyz;

	float3 first = lerp(c00, c20, frac(scale_factor * vTexCoord.x * texsize.x + 0.5));
	float3 second = lerp(c02, c22, frac(scale_factor * vTexCoord.x * texsize.x + 0.5));

	float3 mid_horiz = lerp(c01, c21, frac(scale_factor * vTexCoord.x * texsize.x + 0.5));
	float3 mid_vert = lerp(c10, c12, frac(scale_factor * vTexCoord.y * texsize.y + 0.5));

	float3 res = lerp(first, second, frac(scale_factor * vTexCoord.y * texsize.y + 0.5));

	float color =  float4(0.28 * (res + mid_horiz + mid_vert) + 4.7 * abs(res - lerp(mid_horiz, mid_vert, 0.5)), 1.0).x;

   FragColor = float4((color + color_old) / 2.0, 1.0);
}

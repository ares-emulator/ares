#version 450

/*
   Hyllian's Data Dependent Triangulation Shader - Extended Method
   
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

	float3 C00 = tex2D(decal, vTexCoord -g1-g2).xyz;
	float3 C01 = tex2D(decal, vTexCoord    -g2).xyz;
	float3 C02 = tex2D(decal, vTexCoord +g1-g2).xyz;
	float3 C03 = tex2D(decal, vTexCoord +2.0*g1-g2).xyz;

	float3 C10 = tex2D(decal, vTexCoord -g1   ).xyz;
	float3 C20 = tex2D(decal, vTexCoord -g1+g2).xyz;
	float3 C13 = tex2D(decal, vTexCoord +2.0*g1   ).xyz;
	float3 C23 = tex2D(decal, vTexCoord +2.0*g1+g2).xyz;

	float3 C30 = tex2D(decal, vTexCoord -g1+2.0*g2).xyz;
	float3 C31 = tex2D(decal, vTexCoord    +2.0*g2).xyz;
	float3 C32 = tex2D(decal, vTexCoord +g1+2.0*g2).xyz;
	float3 C33 = tex2D(decal, vTexCoord +2.0*g1+2.0*g2).xyz;

	float a = reduce(A);
	float b = reduce(B);
	float c = reduce(C);
	float d = reduce(D);

	float c00 = reduce(C00);
	float c01 = reduce(C01);
	float c02 = reduce(C02);
	float c03 = reduce(C03);

	float c10 = reduce(C10);
	float c20 = reduce(C20);
	float c13 = reduce(C13);
	float c23 = reduce(C23);

	float c30 = reduce(C30);
	float c31 = reduce(C31);
	float c32 = reduce(C32);
	float c33 = reduce(C33);

	float p = abs(pos.x);
	float q = abs(pos.y);

	float k = distance(pos,g1 * params.SourceSize.xy);
	float l = distance(pos,g2 * params.SourceSize.xy);
	
	float count1 = 0.0;
	float count2 = 0.0;

	count1 += float(abs(c00-a) < abs(c01-c10));
	count2 += float(abs(c00-a) > abs(c01-c10));
	count1 += float(abs(c01-b) < abs(c02-a));
	count2 += float(abs(c01-b) > abs(c02-a));
	count1 += float(abs(c02-c13) < abs(c03-b));
	count2 += float(abs(c02-c13) > abs(c03-b));
	count1 += float(abs(c10-c) < abs(c20-a));
	count2 += float(abs(c10-c) > abs(c20-a));
	count1 += float(abs(b-c23) < abs(c13-d));
	count2 += float(abs(b-c23) > abs(c13-d));
	count1 += float(abs(c20-c31) < abs(c-c30));
	count2 += float(abs(c20-c31) > abs(c-c30));
	count1 += float(abs(c32-c) < abs(c31-d));
	count2 += float(abs(c32-c) > abs(c31-d));
	count1 += float(abs(c33-d) < abs(c32-c23));
	count2 += float(abs(c33-d) > abs(c32-c23));

	if ((count1 >= 5.0) || ((abs(a-d) < abs(b-c)) && count2 <= 4.0))
	{
		if (k < l)
		{
			C = A + D - B;
		}
		else if (k >= l)
		{
			B = A + D - C;
		}
	}
	else if ((count2 >= 5.0) || (abs(a-d) > abs(b-c)))
	{
		D = B + C - A;
	}

	float3 color = bilinear(p, q, A, B, C, D);

   FragColor = float4(color, 1.0);
}

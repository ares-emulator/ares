#version 450

/*
   Hyllian's Fast Bilateral Shader
   
   Copyright (C) 2011/2016 Hyllian - sergiogdb@gmail.com

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
	float SIGMA_R;
} params;

#pragma parameter SIGMA_R "Bilateral Blur" 0.4 0.0 1.0 0.1

#define SIGMA_R params.SIGMA_R

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

#define decal Source

#define GET(M,K) (tex2D(decal,tc+M*dx+K*dy).xyz)

#define BIL(M,K)  {col=GET(M,K);ds=M*M+K*K;weight=exp(-ds/sd2)*exp(-(col-center)*(col-center)/si2);color+=(weight*col);wsum+=weight;}

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
      float ds, sd2, si2;
      float sigma_d = 3.0;
      float sigma_r = SIGMA_R*0.04;

      float3 color = float3(0.0, 0.0, 0.0);
      float3 wsum = float3(0.0, 0.0, 0.0);
      float3 weight;

      float2 dx = float2(1.0, 0.0) * params.SourceSize.zw;
      float2 dy = float2(0.0, 1.0) * params.SourceSize.zw;

      sd2 = 2.0 * sigma_d * sigma_d;
      si2 = 2.0 * sigma_r * sigma_r;

      float2 tc = vTexCoord;

      float3 col;
      float3 center = GET(0,0);

      BIL(-2,-2)
      BIL(-1,-2)
      BIL( 0,-2)
      BIL( 1,-2)
      BIL( 2,-2)
      BIL(-2,-1)
      BIL(-1,-1)
      BIL( 0,-1)
      BIL( 1,-1)
      BIL( 2,-1)
      BIL(-2, 0)
      BIL(-1, 0)
      BIL( 0, 0)
      BIL( 1, 0)
      BIL( 2, 0)
      BIL(-2, 1)
      BIL(-1, 1)
      BIL( 0, 1)
      BIL( 1, 1)
      BIL( 2, 1)
      BIL(-2, 2)
      BIL(-1, 2)
      BIL( 0, 2)
      BIL( 1, 2)
      BIL( 2, 2)

      // Weight normalization
      color /= wsum;
   FragColor = vec4(color, 1.0);
}
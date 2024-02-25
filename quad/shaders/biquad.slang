#version 450

/*
   Hyllian's biquad Shader
   
   Copyright (C) 2011-2015 Hyllian/Jararaca - sergiogdb@gmail.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float K;
} params;


#pragma parameter K "Blurring Param" 0.8 0.0 1.0 0.01

#define K params.K

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
#define s_p Source

// Calculates the distance between two points
float d(float2 pt1, float2 pt2)
{
  float2 v = pt2 - pt1;
  return sqrt(dot(v,v));
}

float3 resampler(float3 x)
    {
      float3 res;

      res = lessThanEqual(x,float3(0.5, 0.5, 0.5)) == bvec3(true) ?  (-2*K*x*x + 0.5*(K+1))  :  (lessThanEqual(x,float3(1.5, 1.5, 1.5)) == bvec3(true)  ?  (K*x*x + (-2*K - 0.5)*x + 0.75*(K+1))  :  float3(0.00001, 0.00001, 0.00001));

      return res;
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
      float3 color;
      float3x3 weights;

      float2 dx = float2(1.0, 0.0);
      float2 dy = float2(0.0, 1.0);

      float2 pc = vTexCoord*params.SourceSize.xy;

      float2 tc = (floor(pc)+float2(0.5,0.5));
     
      weights[0] = resampler(float3(d(pc, tc    -dx    -dy), d(pc, tc           -dy), d(pc, tc    +dx    -dy)));
      weights[1] = resampler(float3(d(pc, tc    -dx       ), d(pc, tc              ), d(pc, tc    +dx       )));
      weights[2] = resampler(float3(d(pc, tc    -dx    +dy), d(pc, tc           +dy), d(pc, tc    +dx    +dy)));

      dx = dx * params.SourceSize.zw;
      dy = dy * params.SourceSize.zw;
      tc = tc * params.SourceSize.zw;
     
     // reading the texels
      float3 c00 = tex2D(s_p, tc    -dx    -dy).xyz;
      float3 c10 = tex2D(s_p, tc           -dy).xyz;
      float3 c20 = tex2D(s_p, tc    +dx    -dy).xyz;
      float3 c01 = tex2D(s_p, tc    -dx       ).xyz;
      float3 c11 = tex2D(s_p, tc              ).xyz;
      float3 c21 = tex2D(s_p, tc    +dx       ).xyz;
      float3 c02 = tex2D(s_p, tc    -dx    +dy).xyz;
      float3 c12 = tex2D(s_p, tc           +dy).xyz;
      float3 c22 = tex2D(s_p, tc    +dx    +dy).xyz;

      color = mul(weights[0], float3x3(c00, c10, c20));
      color+= mul(weights[1], float3x3(c01, c11, c21));
      color+= mul(weights[2], float3x3(c02, c12, c22));
      color = color/(dot(mul(weights, float3(1.0)), float3(1.0)));

      // final sum and weight normalization
   FragColor = vec4(color, 1.0);
}
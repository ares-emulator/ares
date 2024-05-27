#version 450

/*
   Hyllian's jinc windowed-jinc 2-lobe with anti-ringing Shader
   
   Copyright (C) 2011-2016 Hyllian - sergiogdb@gmail.com

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

      /*
         This is an approximation of Jinc(x)*Jinc(x*r1/r2) for x < 2.5,
         where r1 and r2 are the first two zeros of jinc function.
         For a jinc 2-lobe best approximation, use A=0.5 and B=0.825.
      */  

// A=0.5, B=0.825 is the best jinc approximation for x<2.5. if B=1.0, it's a lanczos filter.
// Increase A to get more blur. Decrease it to get a sharper picture. 
// B = 0.825 to get rid of dithering. Increase B to get a fine sharpness, though dithering returns.
	  
layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float NEDI_JINC2_WINDOW_SINC;
	float NEDI_JINC2_SINC;
	float NEDI_JINC2_AR_STRENGTH;
} params;

#pragma parameter NEDI_JINC2_WINDOW_SINC "Window Sinc Param" 0.46 0.0 1.0 0.01
#pragma parameter NEDI_JINC2_SINC "Sinc Param" 0.88 0.0 1.0 0.01
#pragma parameter NEDI_JINC2_AR_STRENGTH "Anti-ringing Strength" 0.0 0.0 1.0 0.1

#define JINC2_WINDOW_SINC params.NEDI_JINC2_WINDOW_SINC
#define JINC2_SINC params.NEDI_JINC2_SINC
#define JINC2_AR_STRENGTH params.NEDI_JINC2_AR_STRENGTH

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define saturate(c) clamp(c, 0.0, 1.0)
#define lerp(a,b,c) mix(a,b,c)
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
#define float4x3 mat4x3

#define halfpi  1.5707963267948966192313216916398
#define pi    3.1415926535897932384626433832795
#define wa    (JINC2_WINDOW_SINC*pi)
#define wb    (JINC2_SINC*pi)

const float3 Y = float3(0.299, 0.587, 0.114);

float df(float A, float B)
{
	return abs(A-B);
}

// Calculates the distance between two points
float d(float2 pt1, float2 pt2)
{
  float2 v = pt2 - pt1;
  return sqrt(dot(v,v));
}

float3 min4(float3 a, float3 b, float3 c, float3 d)
{
    return min(a, min(b, min(c, d)));
}

float3 max4(float3 a, float3 b, float3 c, float3 d)
{
    return max(a, max(b, max(c, d)));
}

 float4 resampler(float4 x)
    {
      float4 res;

      res.x = (x.x==0.0) ?  (wa*wb)  :  sin(x.x*wa)*sin(x.x*wb)/(x.x*x.x);
      res.y = (x.y==0.0) ?  (wa*wb)  :  sin(x.y*wa)*sin(x.y*wb)/(x.y*x.y);
      res.z = (x.z==0.0) ?  (wa*wb)  :  sin(x.z*wa)*sin(x.z*wb)/(x.z*x.z);
      res.w = (x.w==0.0) ?  (wa*wb)  :  sin(x.w*wa)*sin(x.w*wb)/(x.w*x.w);

      return res;
    }

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.00001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
      float3 color;
      float4x4 weights;

      float2 dx = float2(1.0, 0.0);
      float2 dy = float2(0.0, 1.0);

      float2 pc = vTexCoord*params.SourceSize.xy;

      float2 tc = (floor(pc-float2(0.4999,0.4999))+float2(0.4999,0.4999));
     
      weights[0] = resampler(float4(d(pc, tc    -dx    -dy), d(pc, tc           -dy), d(pc, tc    +dx    -dy), d(pc, tc+2.0*dx    -dy)));
      weights[1] = resampler(float4(d(pc, tc    -dx       ), d(pc, tc              ), d(pc, tc    +dx       ), d(pc, tc+2.0*dx       )));
      weights[2] = resampler(float4(d(pc, tc    -dx    +dy), d(pc, tc           +dy), d(pc, tc    +dx    +dy), d(pc, tc+2.0*dx    +dy)));
      weights[3] = resampler(float4(d(pc, tc    -dx+2.0*dy), d(pc, tc       +2.0*dy), d(pc, tc    +dx+2.0*dy), d(pc, tc+2.0*dx+2.0*dy)));

      //weights[0][0] = weights[0][3] = weights[3][0] = weights[3][3] = 0.0;

      dx = dx * params.SourceSize.zw;
      dy = dy * params.SourceSize.zw;
      tc = tc * params.SourceSize.zw;
     
     // reading the texels
     
      float3 c00 = tex2D(Source, tc    -dx    -dy).xyz;
      float3 c10 = tex2D(Source, tc           -dy).xyz;
      float3 c20 = tex2D(Source, tc    +dx    -dy).xyz;
      float3 c30 = tex2D(Source, tc+2.0*dx    -dy).xyz;
      float3 c01 = tex2D(Source, tc    -dx       ).xyz;
      float3 c11 = tex2D(Source, tc              ).xyz;
      float3 c21 = tex2D(Source, tc    +dx       ).xyz;
      float3 c31 = tex2D(Source, tc+2.0*dx       ).xyz;
      float3 c02 = tex2D(Source, tc    -dx    +dy).xyz;
      float3 c12 = tex2D(Source, tc           +dy).xyz;
      float3 c22 = tex2D(Source, tc    +dx    +dy).xyz;
      float3 c32 = tex2D(Source, tc+2.0*dx    +dy).xyz;
      float3 c03 = tex2D(Source, tc    -dx+2.0*dy).xyz;
      float3 c13 = tex2D(Source, tc       +2.0*dy).xyz;
      float3 c23 = tex2D(Source, tc    +dx+2.0*dy).xyz;
      float3 c33 = tex2D(Source, tc+2.0*dx+2.0*dy).xyz;



      color = mul(weights[0], float4x3(c00, c10, c20, c30));
      color+= mul(weights[1], float4x3(c01, c11, c21, c31));
      color+= mul(weights[2], float4x3(c02, c12, c22, c32));
      color+= mul(weights[3], float4x3(c03, c13, c23, c33));
      color = color/(dot(mul(weights, float4(1.0)), float4(1.0)));



      // Anti-ringing
      //  Get min/max samples

      float3 min_sample = min4(c11, c21, c12, c22);
      float3 max_sample = max4(c11, c21, c12, c22);

      float3 aux = color;
      color = clamp(color, min_sample, max_sample);

      color = mix(aux, color, JINC2_AR_STRENGTH);
 
      // final sum and weight normalization
   FragColor = vec4(color, 1.0);
}

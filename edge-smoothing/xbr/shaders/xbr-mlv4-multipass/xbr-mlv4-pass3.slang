#version 450

/*

   Hyllian's xBR MultiLevel4 Shader - Pass3
   
   Copyright (C) 2011-2015 Hyllian - sergiogdb@gmail.com

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
	vec4 REFSize;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// compatibility macros
#define mul(a,b) (b*a)
#define saturate(c) clamp(c, 0.0, 1.0)
#define frac(x) (fract(x))
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4
#define float3x3 mat3x3
#define float4x3 mat4x3
#define float2x4 mat2x4
#define texture_size params.SourceSize
#define video_size params.SourceSize
#define output_size params.OutputSize

#define round(X) floor((X)+0.5)

const float coef            = 2.0;
const float cf              = 4.0;
const float eq_threshold    = 15.0;
const float y_weight        = 48.0;
const float u_weight        = 7.0;
const float v_weight        = 6.0;
const float3x3 yuv          = float3x3(0.299, 0.587, 0.114, -0.169, -0.331, 0.499, 0.499, -0.418, -0.0813);
const float3x3 yuv_weighted = float3x3(y_weight*yuv[0], u_weight*yuv[1], v_weight*yuv[2]);
const float4 maximo         = float4(255.0, 255.0, 255.0, 255.0);
const float4 low            = float4(-64.0, -64.0, -64.0, -64.0);
const float4 high           = float4( 64.0,  64.0,  64.0,  64.0);


const float2x4 sym_vectors  = float2x4(1.,  1.,   -1., -1.,    1., -1.,   -1.,  1.);

// Bx, Ay, C
const float3 lines[13] = float3[](
   float3(4.0, 4.0, 4.0),  //  0  NL
   float3(4.0, 4.0, 3.0),  //  1  LV0
   float3(4.0, 4.0, 2.0),  //  2  LV1
   float3(8.0, 4.0, 2.0),  //  3  LV2u
   float3(4.0, 8.0, 2.0),  //  4  LV2l
  float3(12.0, 4.0, 2.0),  //  5  LV3u
   float3(4.0,12.0, 2.0),  //  6  LV3l
  float3(16.0, 4.0, 2.0),  //  7  LV4u
   float3(4.0,16.0, 2.0),  //  8  LV4l

  float3(12.0, 4.0, 6.0),  //  9  LV3u
   float3(4.0,12.0, 6.0),  // 10  LV3l
  float3(16.0, 4.0, 6.0),  // 11  LV4u
   float3(4.0,16.0, 6.0)  // 12  LV4l
);

float4 remapTo01(float4 v, float4 low, float4 high)
{
	return saturate((v - low)/(high-low));
}

float remapFrom01(float v, float high)
{
	return round(high*v);
}

float df(float A, float B)
{
	return abs(A-B);
}

bool eq(float A, float B)
{
	return (df(A, B) < eq_threshold);
}

float weighted_distance(float a, float b, float c, float d, float e, float f, float g, float h)
{
	return (df(a,b) + df(a,c) + df(d,e) + df(d,f) + 4.0*df(g,h));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0004;
	float2 ps = float2(1.0/params.REFSize.x, 1.0/params.REFSize.y);
	float dx = ps.x;
	float dy = ps.y;

	//      A3 B3 C3
	//      A1 B1 C1
	//A2 A0  A  B  C C4 C6
	//D2 D0  D  E  F F4 F6
	//G2 G0  G  H  I I4 I6
	//      G5 H5 I5
	//      G7 H7 I7

	t1       = float4(dx, 0., 0., dy);  // F  H
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D REF;

void main()
{
	float px;

	float2 pos = frac(vTexCoord*params.REFSize.xy)-float2(0.5, 0.5); // pos = pixel position
	float2 dir = sign(pos); // dir = pixel direction

	float2 g1 = dir*( saturate(-dir.y*dir.x)*t1.zw + saturate( dir.y*dir.x)*t1.xy);
	float2 g2 = dir*( saturate( dir.y*dir.x)*t1.zw + saturate(-dir.y*dir.x)*t1.xy);

	float3 E   = texture(REF, vTexCoord    ).rgb;
	float3 F   = texture(REF, vTexCoord +g1).rgb;
	float3 H   = texture(REF, vTexCoord +g2).rgb;
	float3 I   = texture(REF, vTexCoord +g1+g2).rgb;
	float3 F4  = texture(REF, vTexCoord +2.0*g1).rgb;
	float3 H5  = texture(REF, vTexCoord +2.0*g2).rgb;

	float e = dot(E, yuv_weighted[0]);
	float f = dot(F, yuv_weighted[0]);
	float h = dot(H, yuv_weighted[0]);
	float i = dot(I, yuv_weighted[0]);
	float f4= dot(F4, yuv_weighted[0]);
	float h5= dot(H5, yuv_weighted[0]);

	float4 icomp    = round(saturate(mul(dir, sym_vectors))); // choose info component

	float  infoE    = remapFrom01(dot(texture(Source, vTexCoord   ), icomp), 255.0); // retrieve 1st pass info
	float  infoF    = remapFrom01(dot(texture(Source, vTexCoord+g1), icomp), 255.0); // 1st pass info from neighbor r
	float  infoH    = remapFrom01(dot(texture(Source, vTexCoord+g2), icomp), 255.0); // 1st pass info from neighbor d

	float4 lparam;
	float2 addr;

	if (infoF == 8.0)
	{
		lparam.xyz = lines[12];
		px = float(df(f,f4) <= df(f,i));
		addr.x = 2.*px + saturate(1.0-px);
		addr.y = saturate(1.0-px);
	}
	else if (infoH == 7.0)
	{
		lparam.xyz = lines[11];
		px = float(df(h,h5) <= df(h,i));
		addr.x = saturate(1.0-px);
		addr.y = 2.*px + saturate(1.0-px);
	}
	else if (infoF == 6.0)
	{
		lparam.xyz = lines[10];
		px = float(df(f,f4) <= df(f,i));
		addr.x = 2.*px + saturate(1.0-px);
		addr.y = saturate(1.0-px);
	}
	else if (infoH == 5.0)
	{
		lparam.xyz = lines[9];
		px = float(df(h,h5) <= df(h,i));
		addr.x = saturate(1.0-px);
		addr.y = 2.*px + saturate(1.0-px);
	}
	else
	{
		px = float(df(e,f) <= df(e,h));
		addr.x = px;
		addr.y = saturate(1.0-px);

		lparam.xyz = (infoE == 8.0) ? lines[8] : ((infoE == 7.0) ? lines[7] : ((infoE == 6.0) ? lines[6] : ((infoE == 5.0) ? lines[5] : ((infoE == 4.0) ? lines[4] : ((infoE == 3.0) ? lines[3] : ((infoE == 2.0) ? lines[2] : ((infoE == 1.0) ? lines[1] : lines[0])))))));
	}

	bool inv = (dir.x*dir.y) < 0.0 ? true : false;

	// Rotate address from relative to absolute.
	addr = addr*dir.yx;
	addr = inv ? addr.yx : addr;

	// Rotate straight line equation from relative to absolute.
	lparam.xy = lparam.xy*dir.yx;
	lparam.xy = inv ? lparam.yx : lparam.xy;

	addr.x+=2.0;
	addr.y+=2.0;

	lparam.w = addr.x*8.0 + addr.y;

    FragColor = float4(remapTo01(lparam, low, high));
}

/*
19 1
9  1
4  0
2  0
1  1
0  0

0 0000   ND
1 0001   EDR0
2 0010   EDR
3 0011   EDRU
4 0100   EDRL
5 0101   EDRU3
6 0110   EDRL3

0   1 2 3 4
-2 -1 0 1 2

*/

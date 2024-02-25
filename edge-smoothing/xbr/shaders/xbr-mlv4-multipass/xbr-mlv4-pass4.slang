#version 450

/*

   Hyllian's xBR MultiLevel4 Shader - Pass4
   
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
#define lerp(a,b,c) mix(a,b,c)
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

const float3 bin            = float3( 4.0,  2.0,  1.0);
const float4 low            = float4(-64.0, -64.0, -64.0, -64.0);
const float4 high           = float4( 64.0,  64.0,  64.0,  64.0);

const float2x4 sym_vectors  = float2x4(1.,  1.,   -1., -1.,    1., -1.,   -1.,  1.);

float4 remapFrom01(float4 v, float4 low, float4 high)
{
	return round(lerp(low, high, v));
}

float c_df(float3 c1, float3 c2)
{
	float3 df = abs(c1 - c2);
	return df.r + df.g + df.b;
}


float4 unpack_info(float i)
{
	float4 info;
	info.x = round(modf(i/2.0, i));
	info.y = round(modf(i/2.0, i));
	info.z = round(modf(i/2.0, i));
	info.w = i;

	return info;
}


float df(float A, float B)
{
	return abs(A-B);
}

#define GET_PIXEL(PARAM, PIXEL)\
	info = PARAM;\
	ay.z = round(  modf( info/1.9999, info )  );\
	ay.y = round(  modf( info/1.9999, info )  );\
	ay.x = round(  modf( info/1.9999, info )  );\
	ax.z = round(  modf( info/1.9999, info )  );\
	ax.y = round(  modf( info/1.9999, info )  );\
	ax.x = round(  info  );\
	iq.x = dot( ax, bin ) - 1.9999;\
	iq.y = dot( ay, bin ) - 1.9999;\
	PIXEL = texture( REF, vTexCoord + iq.x*t1.xy + iq.y*t1.zw ).xyz;\

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out float scale_factor;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
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

	t1           = float4(dx, 0., 0., dy);  // F  H
	scale_factor = output_size.x/params.REFSize.x;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in float scale_factor;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D REF;

void main()
{
	float2 fp = frac( vTexCoord*params.REFSize.xy ) - float2( 0.5, 0.5 ); // pos = pixel position

	float2 pxcoord = floor(vTexCoord*params.REFSize.xy)/params.REFSize.xy;

	float4 UL = texture(Source, pxcoord + 0.25*t1.xy + 0.25*t1.zw );
	float4 UR = texture(Source, pxcoord + 0.75*t1.xy + 0.25*t1.zw );
	float4 DL = texture(Source, pxcoord + 0.25*t1.xy + 0.75*t1.zw );
	float4 DR = texture(Source, pxcoord + 0.75*t1.xy + 0.75*t1.zw );

	float4 ulparam = remapFrom01( UL, low, high ); // retrieve 1st pass info
	float4 urparam = remapFrom01( UR, low, high ); // retrieve 1st pass info
	float4 dlparam = remapFrom01( DL, low, high ); // retrieve 1st pass info
	float4 drparam = remapFrom01( DR, low, high ); // retrieve 1st pass info

	float3 E = texture( REF, vTexCoord ).xyz;

	float3 ax, ay, PX, PY, PZ, PW;
	float info;
	float2 iq;

#ifdef DEBUG
	PX = unpack_info(ulparam.w).xyz;
	PY = unpack_info(urparam.w).xyz;
	PZ = unpack_info(dlparam.w).xyz;
	PW = unpack_info(drparam.w).xyz;
#else	
	GET_PIXEL(ulparam.w, PX);
	GET_PIXEL(urparam.w, PY);
	GET_PIXEL(dlparam.w, PZ);
	GET_PIXEL(drparam.w, PW);
#endif

	float3 fp1 = float3( fp, -1. );

	float3 color;
	float4 fx;

	float4 inc   = float4(abs(ulparam.x/ulparam.y), abs(urparam.x/urparam.y), abs(dlparam.x/dlparam.y), abs(drparam.x/drparam.y));
	float4 level = max(inc, 1.0/inc);

	fx.x    = saturate( dot( fp1, ulparam.xyz ) * scale_factor/( 8. * level.x ) + 0.5 );
	fx.y    = saturate( dot( fp1, urparam.xyz ) * scale_factor/( 8. * level.y ) + 0.5 );
	fx.z    = saturate( dot( fp1, dlparam.xyz ) * scale_factor/( 8. * level.z ) + 0.5 );
	fx.w    = saturate( dot( fp1, drparam.xyz ) * scale_factor/( 8. * level.w ) + 0.5 );

	float3 c1, c2, c3, c4;

	c1 = lerp( E, PX, fx.x );
	c2 = lerp( E, PY, fx.y );
	c3 = lerp( E, PZ, fx.z );
	c4 = lerp( E, PW, fx.w );

	color = c1;
	color = ( (c_df(c2, E) > c_df(color, E)) ) ? c2 : color;
	color = ( (c_df(c3, E) > c_df(color, E)) ) ? c3 : color;
	color = ( (c_df(c4, E) > c_df(color, E)) ) ? c4 : color;

    FragColor = float4( color, 1.0 );
}

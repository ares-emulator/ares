#version 450

/*
	SABR v3.0 Shader
	Joshua Street
	
	Portions of this algorithm were taken from Hyllian's 5xBR v3.7c
	shader.
	
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
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define saturate(c) clamp(c, 0.0, 1.0)
#define lerp(a,b,c) mix(a,b,c)
#define mul(a,b) (b*a)
#define fmod(a,b) mod(a,b)
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

#define decal Original

const  float4 Ai  = float4( 1.0, -1.0, -1.0,  1.0);
const  float4 B45 = float4( 1.0,  1.0, -1.0, -1.0);
const  float4 C45 = float4( 1.5,  0.5, -0.5,  0.5);
const  float4 B30 = float4( 0.5,  2.0, -0.5, -2.0);
const  float4 C30 = float4( 1.0,  1.0, -0.5,  0.0);
const  float4 B60 = float4( 2.0,  0.5, -2.0, -0.5);
const  float4 C60 = float4( 2.0,  0.0, -1.0,  0.5);

const  float4 M45 = float4(0.4, 0.4, 0.4, 0.4);
const  float4 M30 = float4(0.2, 0.4, 0.2, 0.4);
const  float4 M60 = M30.yxwz;
const  float4 Mshift = float4(0.2, 0.2, 0.2, 0.2);

const  float coef = 2.0;

const  float4 threshold = float4(0.32, 0.32, 0.32, 0.32);

const  float3 lum = float3(0.21, 0.72, 0.07);

float4 lum_to(float3 v0, float3 v1, float3 v2, float3 v3) {
	return float4(dot(lum, v0), dot(lum, v1), dot(lum, v2), dot(lum, v3));
}

float4 lum_df(float4 A, float4 B) {
	return abs(A - B);
}

bool4 lum_eq(float4 A, float4 B) {
	return lessThan(lum_df(A, B) , float4(threshold));
}

float4 lum_wd(float4 a, float4 b, float4 c, float4 d, float4 e, float4 f, float4 g, float4 h) {
	return lum_df(a, b) + lum_df(a, c) + lum_df(d, e) + lum_df(d, f) + 4.0 * lum_df(g, h);
}

float c_df(float3 c1, float3 c2) {
	float3 df = abs(c1 - c2);
	return df.r + df.g + df.b;
}


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 tc;
layout(location = 1) out vec4 xyp_1_2_3;
layout(location = 2) out vec4 xyp_5_10_15;
layout(location = 3) out vec4 xyp_6_7_8;
layout(location = 4) out vec4 xyp_9_14_9;
layout(location = 5) out vec4 xyp_11_12_13;
layout(location = 6) out vec4 xyp_16_17_18;
layout(location = 7) out vec4 xyp_21_22_23;

void main()
{
   gl_Position = global.MVP * Position;
   
   	float x = params.SourceSize.z;//1.0 / IN.texture_size.x;
	float y = params.SourceSize.w;//1.0 / IN.texture_size.y;
	
	tc = TexCoord * 1.00001;
	xyp_1_2_3    = tc.xxxy + float4(      -x, 0.0,   x, -2.0 * y);
	xyp_6_7_8    = tc.xxxy + float4(      -x, 0.0,   x,       -y);
	xyp_11_12_13 = tc.xxxy + float4(      -x, 0.0,   x,      0.0);
	xyp_16_17_18 = tc.xxxy + float4(      -x, 0.0,   x,        y);
	xyp_21_22_23 = tc.xxxy + float4(      -x, 0.0,   x,  2.0 * y);
	xyp_5_10_15  = tc.xyyy + float4(-2.0 * x,  -y, 0.0,        y);
	xyp_9_14_9   = tc.xyyy + float4( 2.0 * x,  -y, 0.0,        y);
}

#pragma stage fragment
layout(location = 0) in vec2 tc;
layout(location = 1) in vec4 xyp_1_2_3;
layout(location = 2) in vec4 xyp_5_10_15;
layout(location = 3) in vec4 xyp_6_7_8;
layout(location = 4) in vec4 xyp_9_14_9;
layout(location = 5) in vec4 xyp_11_12_13;
layout(location = 6) in vec4 xyp_16_17_18;
layout(location = 7) in vec4 xyp_21_22_23;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Original;

void main()
{
	/*
		Mask for algorithm
		+-----+-----+-----+-----+-----+
		|     |  1  |  2  |  3  |     |
		+-----+-----+-----+-----+-----+
		|  5  |  6  |  7  |  8  |  9  |
		+-----+-----+-----+-----+-----+
		| 10  | 11  | 12  | 13  | 14  |
		+-----+-----+-----+-----+-----+
		| 15  | 16  | 17  | 18  | 19  |
		+-----+-----+-----+-----+-----+
		|     | 21  | 22  | 23  |     |
		+-----+-----+-----+-----+-----+
	*/
	// Store mask values
	float3 P1  = tex2D(decal, xyp_1_2_3.xw   ).rgb;
	float3 P2  = tex2D(decal, xyp_1_2_3.yw   ).rgb;
	float3 P3  = tex2D(decal, xyp_1_2_3.zw   ).rgb;
	
	float3 P6  = tex2D(decal, xyp_6_7_8.xw   ).rgb;
	float3 P7  = tex2D(decal, xyp_6_7_8.yw   ).rgb;
	float3 P8  = tex2D(decal, xyp_6_7_8.zw   ).rgb;
	
	float3 P11 = tex2D(decal, xyp_11_12_13.xw).rgb;
	float3 P12 = tex2D(decal, xyp_11_12_13.yw).rgb;
	float3 P13 = tex2D(decal, xyp_11_12_13.zw).rgb;
	
	float3 P16 = tex2D(decal, xyp_16_17_18.xw).rgb;
	float3 P17 = tex2D(decal, xyp_16_17_18.yw).rgb;
	float3 P18 = tex2D(decal, xyp_16_17_18.zw).rgb;
	
	float3 P21 = tex2D(decal, xyp_21_22_23.xw).rgb;
	float3 P22 = tex2D(decal, xyp_21_22_23.yw).rgb;
	float3 P23 = tex2D(decal, xyp_21_22_23.zw).rgb;
	
	float3 P5  = tex2D(decal, xyp_5_10_15.xy ).rgb;
	float3 P10 = tex2D(decal, xyp_5_10_15.xz ).rgb;
	float3 P15 = tex2D(decal, xyp_5_10_15.xw ).rgb;
	
	float3 P9  = tex2D(decal, xyp_9_14_9.xy  ).rgb;
	float3 P14 = tex2D(decal, xyp_9_14_9.xz  ).rgb;
	float3 P19 = tex2D(decal, xyp_9_14_9.xw  ).rgb;
	
	// Store luminance values of each point 
	float4 p7  = lum_to(P7,  P11, P17, P13);
	float4 p8  = lum_to(P8,  P6,  P16, P18);
	float4 p11 = p7.yzwx;                      // P11, P17, P13, P7
	float4 p12 = lum_to(P12, P12, P12, P12);
	float4 p13 = p7.wxyz;                      // P13, P7,  P11, P17
	float4 p14 = lum_to(P14, P2,  P10, P22);
	float4 p16 = p8.zwxy;                      // P16, P18, P8,  P6
	float4 p17 = p7.zwxy;                      // P11, P17, P13, P7
	float4 p18 = p8.wxyz;                      // P18, P8,  P6,  P16
	float4 p19 = lum_to(P19, P3,  P5,  P21);
	float4 p22 = p14.wxyz;                     // P22, P14, P2,  P10
	float4 p23 = lum_to(P23, P9,  P1,  P15);
	
	float2 fp = frac(tc * params.SourceSize.xy);
	
	float4 ma45 = smoothstep(C45 - M45, C45 + M45, Ai * fp.y + B45 * fp.x);
	float4 ma30 = smoothstep(C30 - M30, C30 + M30, Ai * fp.y + B30 * fp.x);
	float4 ma60 = smoothstep(C60 - M60, C60 + M60, Ai * fp.y + B60 * fp.x);
	float4 marn = smoothstep(C45 - M45 + Mshift, C45 + M45 + Mshift, Ai * fp.y + B45 * fp.x);
	
	float4 e45   = lum_wd(p12, p8, p16, p18, p22, p14, p17, p13);
	float4 econt = lum_wd(p17, p11, p23, p13, p7, p19, p12, p18);
	float4 e30   = lum_df(p13, p16);
	float4 e60   = lum_df(p8, p17);

/*  The whole edge detection thing seems broken here, so may as well comment it out to save cycles	
	bool4 r45 = (notEqual(p12 , p13) == bool4(true) && notEqual(p12 , p17) == bool4(true) && (
			lum_eq(p13, p7) == bool4(false) && lum_eq(p13, p8) == bool4(false) ||
			lum_eq(p17, p11) == bool4(false) && lum_eq(p17, p16) == bool4(false) ||
			lum_eq(p12, p18) == bool4(true) && (
					lum_eq(p13, p14) == bool4(false) && lum_eq(p13, p19) == bool4(false) ||
					lum_eq(p17, p22) == bool4(false) && lum_eq(p17, p23) == bool4(false)) ||
			lum_eq(p12, p16) == bool4(true) ||
			lum_eq(p12, p8) == bool4(true))) ? bool4(true) : bool4(false);
	bool4 r30 = (notEqual(p12 , p16) == bool4(true) && notEqual(p11 , p16) == bool4(true)) ? bool4(true) : bool4(false);
	bool4 r60 = (notEqual(p12 , p8) == bool4(true) && notEqual(p7 , p8) == bool4(true)) ? bool4(true) : bool4(false);
	
	bool4 edr45 = (lessThan(e45 , econt) == bool4(true) && r45 == bool4(true)) ? bool4(true) : bool4(false);
	bool4 edrrn = (lessThanEqual(e45 , econt) == bool4(true)) ? bool4(true) : bool4(false);
	bool4 edr30 = (lessThanEqual(e30 * coef , e60) == bool4(true) && (r30 == bool4(true))) ? bool4(true) : bool4(false);
	bool4 edr60 = (lessThanEqual(e60 * coef , e30) == bool4(true) && (r60 == bool4(true))) ? bool4(true) : bool4(false);
*/
	
	//FIXME: dunno what's up here. final45 is the only one that seems to matter and it's either all on or all off
	//       again, may as well comment it out to save cycles :/
	float4 final45 = float4(1.0);//(edr30 == bool4(false) && edr60 == bool4(false) && edr45 == bool4(true)) ? float4(1.0) : float4(1.0);
	float4 final30 = float4(0.0);//(edr45 == bool4(true) && edr30 == bool4(true) && edr60 == bool4(false)) ? float4(1.0) : float4(0.0);
	float4 final60 = float4(0.0);//(edr45 == bool4(true) && edr60 == bool4(true) && edr30 == bool4(false)) ? float4(1.0) : float4(0.0);
	float4 final36 = float4(0.0);//(edr45 == bool4(true) && edr30 == bool4(true) && edr60 == bool4(true)) ? float4(1.0) : float4(0.0);
	float4 finalrn = float4(0.0);//(edr45 == bool4(false) && edrrn == bool4(true)) ? float4(1.0) : float4(0.0);
	
	float4 px = step(lum_df(p12, p17), lum_df(p12, p13));
	
	float4 mac = final36 * max(ma30, ma60) + final30 * ma30 + final60 * ma60 + final45 * ma45 + finalrn * marn;
	
	float3 res1 = P12;
	res1 = lerp(res1, lerp(P13, P17, px.x), mac.x);
	res1 = lerp(res1, lerp(P7 , P13, px.y), mac.y);
	res1 = lerp(res1, lerp(P11, P7 , px.z), mac.z);
	res1 = lerp(res1, lerp(P17, P11, px.w), mac.w);
	
	float3 res2 = P12;
	res2 = lerp(res2, lerp(P17, P11, px.w), mac.w);
	res2 = lerp(res2, lerp(P11, P7 , px.z), mac.z);
	res2 = lerp(res2, lerp(P7 , P13, px.y), mac.y);
	res2 = lerp(res2, lerp(P13, P17, px.x), mac.x);
	
   FragColor = float4(lerp(res1, res2, step(c_df(P12, res1), c_df(P12, res2))), 1.0);
}
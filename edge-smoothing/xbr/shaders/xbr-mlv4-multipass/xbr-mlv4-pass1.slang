#version 450

/*

   Hyllian's xBR MultiLevel4 Shader - Pass1
   
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
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// compatibility macros
#define mul(a,b) (b*a)
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4
#define float3x3 mat3x3
#define float4x3 mat4x3
#define texture_size params.SourceSize
#define video_size params.SourceSize
#define output_size params.OutputSize

#define round(X) floor((X)+0.5)
#define TEX(dx,dy) texture(Source, vTexCoord+float2((dx),(dy))*t1).rgb

const float cf2             = 2.0;
const float cf3             = 4.0;
const float cf4             = 4.0;
const float4 eq_thresholdold= float4(15.0, 15.0, 15.0, 15.0);
const float4 eq_threshold   = float4( 2.0,  2.0,  2.0,  2.0);
const float4 eq_threshold3  = float4(25.0, 25.0, 25.0, 25.0);
const float3 Yuv_weight     = float3(4.0, 1.0, 2.0);
const float3x3 yuv          = float3x3(0.299, 0.587, 0.114, -0.169, -0.331, 0.499, 0.499, -0.418, -0.0813);
const float3x3 yuvT         = float3x3(0.299, -0.169,  0.499,
                                              0.587, -0.331, -0.418,
                                              0.114,  0.499, -0.0813); 
const float3x3 yuv_weighted = float3x3(Yuv_weight.x*yuv[0], Yuv_weight.y*yuv[1], Yuv_weight.z*yuv[2]);
const float4 maximo         = float4(255.0, 255.0, 255.0, 255.0);

float4 df(float4 A, float4 B)
{
	return float4(abs(A-B));
}

bool4 rd(float4 A, float4 B, float4 C, float4 D)
{
    return greaterThan((df(C,D)/(df(A,B)+0.000000001)) , vec4(2.,2.,2.,2.));
}

bool4 id(float4 A, float4 B, float4 C, float4 D)
{
    return greaterThan(df(C,D) , df(A,B));
}

float4 remapTo01(float4 v, float4 high)
{
	return (v/high);
}

bool4 eq(float4 A, float4 B)
{
	return lessThan(df(A, B) , eq_threshold);
}

bool4 eq3(float4 A, float4 B)
{
	return lessThan(df(A, B) , eq_threshold3);
}

float4 weighted_distance(float4 a, float4 b, float4 c, float4 d, float4 e, float4 f, float4 g, float4 h)
{
	return (df(a,b) + df(a,c) + df(d,e) + df(d,f) + 4.0*df(g,h));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 t1;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0004;

	float2 ps = float2(1.0/texture_size.x, 1.0/texture_size.y);
	float dx = ps.x;
	float dy = ps.y;

	//      A3 B3 C3
	//      A1 B1 C1
	//A2 A0  A  B  C C4 C6
	//D2 D0  D  E  F F4 F6
	//G2 G0  G  H  I I4 I6
	//      G5 H5 I5
	//      G7 H7 I7
	
	t1       = float2(dx, dy);  // F  H
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 t1;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	bool4 edr0, edr, edr_left, edr_up, edr3_left, edr3_up, edr4_left, edr4_up; // edr = edge detection rule
	bool4 interp_restriction_lv1, interp_restriction_lv2_left, interp_restriction_lv2_up;
	bool4 interp_restriction_lv3_left, interp_restriction_lv3_up;
	bool4 interp_restriction_lv4_left, interp_restriction_lv4_up;

	float3 A3=TEX(-1,-3);	float3 B3=TEX( 0,-3);	float3 C3=TEX( 1,-3);	
	float3 A1=TEX(-1,-2);	float3 B1=TEX( 0,-2);	float3 C1=TEX( 1,-2);
	float3 A2=TEX(-3,-1);	float3 A0=TEX(-2,-1);	float3 A =TEX(-1,-1);
	float3 B =TEX( 0,-1);	float3 C =TEX( 1,-1);	float3 C4=TEX( 2,-1);	float3 C6=TEX( 3,-1);
	float3 D2=TEX(-3, 0);	float3 D0=TEX(-2, 0);	float3 D =TEX(-1, 0);	float3 E =TEX( 0, 0);
	float3 F =TEX( 1, 0);	float3 F4=TEX( 2, 0);	float3 F6=TEX( 3, 0);
	float3 G2=TEX(-3, 1);	float3 G0=TEX(-2, 1);	float3 G =TEX(-1, 1);	float3 H =TEX( 0, 1);
	float3 I =TEX( 1, 1);	float3 I4=TEX( 2, 1);	float3 I6=TEX( 3, 1);
	float3 G5=TEX(-1, 2);	float3 H5=TEX( 0, 2);	float3 I5=TEX( 1, 2);
	float3 G7=TEX(-1, 3);	float3 H7=TEX( 0, 3);	float3 I7=TEX( 1, 3);

	float4x3 bdhf = mul( float4x3(B, D, H, F), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 b = mul( bdhf,  Yuv_weight);

	bdhf = mul( float4x3(C, A, G, I), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 c = mul( bdhf,  Yuv_weight);

	bdhf = mul( float4x3(E, E, E, E), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 e = mul( bdhf,  Yuv_weight);

	float4 d = b.yzwx;
	float4 f = b.wxyz;
	float4 g = c.zwxy;
	float4 h = b.zwxy;
	float4 i = c.wxyz;

	bdhf = mul( float4x3(I4, C1, A0, G5), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 i4 = mul( bdhf,  Yuv_weight);

	bdhf = mul( float4x3(I5, C4, A1, G0), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 i5 = mul( bdhf,  Yuv_weight);

	bdhf = mul( float4x3(H5, F4, B1, D0), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 h5 = mul( bdhf,  Yuv_weight);

	float4 f4 = h5.yzwx;

	float4 c1 = i4.yzwx;
	float4 g0 = i5.wxyz;
	float4 b1 = h5.zwxy;
	float4 d0 = h5.wxyz;

	bdhf = mul( float4x3(I6, C3, A2, G7), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 i6 = mul( bdhf,  Yuv_weight);

	bdhf = mul( float4x3(I7, C6, A3, G2), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 i7 = mul( bdhf,  Yuv_weight);

	bdhf = mul( float4x3(H7, F6, B3, D2), yuvT );
	bdhf = float4x3(abs(bdhf[0]), abs(bdhf[1]), abs(bdhf[2]), abs(bdhf[3]));
	float4 h7 = mul( bdhf,  Yuv_weight);

	float4 f6 = h7.yzwx;

	float4 c3 = i6.yzwx;
	float4 g2 = i7.wxyz;
	float4 b3 = h7.zwxy;
	float4 d2 = h7.wxyz;
	
//	interp_restriction_lv1        = (e  !=f    &&  e  !=h  );
	interp_restriction_lv1.x      = (e.x!=f.x  &&  e.x!=h.x);
	interp_restriction_lv1.y      = (e.y!=f.y  &&  e.y!=h.y);
	interp_restriction_lv1.z      = (e.z!=f.z  &&  e.z!=h.z);
	interp_restriction_lv1.w      = (e.w!=f.w  &&  e.w!=h.w);

//	interp_restriction_lv2_left   = (e  !=g    &&  d  !=g    && (eq(e, d)   || eq(h, g)  ));	
	interp_restriction_lv2_left.x = (e.x!=g.x  &&  d.x!=g.x  && (eq(e, d).x || eq(h, g).x));
	interp_restriction_lv2_left.y = (e.y!=g.y  &&  d.y!=g.y  && (eq(e, d).y || eq(h, g).y));
	interp_restriction_lv2_left.z = (e.z!=g.z  &&  d.z!=g.z  && (eq(e, d).z || eq(h, g).z));
	interp_restriction_lv2_left.w = (e.w!=g.w  &&  d.w!=g.w  && (eq(e, d).w || eq(h, g).w));

//	interp_restriction_lv2_up     = (e  !=c    &&  b  !=c    && (eq(e, b)   || eq(f, c)  ));	
	interp_restriction_lv2_up.x   = (e.x!=c.x  &&  b.x!=c.x  && (eq(e, b).x || eq(f, c).x));
	interp_restriction_lv2_up.y   = (e.y!=c.y  &&  b.y!=c.y  && (eq(e, b).y || eq(f, c).y));
	interp_restriction_lv2_up.z   = (e.z!=c.z  &&  b.z!=c.z  && (eq(e, b).z || eq(f, c).z));
	interp_restriction_lv2_up.w   = (e.w!=c.w  &&  b.w!=c.w  && (eq(e, b).w || eq(f, c).w));

//	interp_restriction_lv3_left   = (e  !=g0   && d0  !=g0   && (eq(d,d0)   || eq(g,g0)  ));	
	interp_restriction_lv3_left.x = (e.x!=g0.x && d0.x!=g0.x && (eq(d,d0).x || eq(g,g0).x));
	interp_restriction_lv3_left.y = (e.y!=g0.y && d0.y!=g0.y && (eq(d,d0).y || eq(g,g0).y));
	interp_restriction_lv3_left.z = (e.z!=g0.z && d0.z!=g0.z && (eq(d,d0).z || eq(g,g0).z));
	interp_restriction_lv3_left.w = (e.w!=g0.w && d0.w!=g0.w && (eq(d,d0).w || eq(g,g0).w));

//	interp_restriction_lv3_up     = (e  !=c1   && b1  !=c1   && (eq(b,b1)   || eq(c,c1)  ));	
	interp_restriction_lv3_up.x   = (e.x!=c1.x && b1.x!=c1.x && (eq(b,b1).x || eq(c,c1).x));
	interp_restriction_lv3_up.y   = (e.y!=c1.y && b1.y!=c1.y && (eq(b,b1).y || eq(c,c1).y));
	interp_restriction_lv3_up.z   = (e.z!=c1.z && b1.z!=c1.z && (eq(b,b1).z || eq(c,c1).z));
	interp_restriction_lv3_up.w   = (e.w!=c1.w && b1.w!=c1.w && (eq(b,b1).w || eq(c,c1).w));

//	interp_restriction_lv4_left   = (e  !=g2   && d2  !=g2   && (eq(d0,d2)   || eq(g0,g2)  ));	
	interp_restriction_lv4_left.x = (e.x!=g2.x && d2.x!=g2.x && (eq(d0,d2).x || eq(g0,g2).x));
	interp_restriction_lv4_left.y = (e.y!=g2.y && d2.y!=g2.y && (eq(d0,d2).y || eq(g0,g2).y));
	interp_restriction_lv4_left.z = (e.z!=g2.z && d2.z!=g2.z && (eq(d0,d2).z || eq(g0,g2).z));
	interp_restriction_lv4_left.w = (e.w!=g2.w && d2.w!=g2.w && (eq(d0,d2).w || eq(g0,g2).w));

//	interp_restriction_lv4_up     = (e  !=c3   && b3  !=c3   && (eq(b1,b3)   || eq(c1,c3)  ));	
	interp_restriction_lv4_up.x   = (e.x!=c3.x && b3.x!=c3.x && (eq(b1,b3).x || eq(c1,c3).x));
	interp_restriction_lv4_up.y   = (e.y!=c3.y && b3.y!=c3.y && (eq(b1,b3).y || eq(c1,c3).y));
	interp_restriction_lv4_up.z   = (e.z!=c3.z && b3.z!=c3.z && (eq(b1,b3).z || eq(c1,c3).z));
	interp_restriction_lv4_up.w   = (e.w!=c3.w && b3.w!=c3.w && (eq(b1,b3).w || eq(c1,c3).w));

	float4 wd1 = weighted_distance( e, c, g, i, h5, f4, h, f);
	float4 wd2 = weighted_distance( h, d, i5, f, i4, b, e, i);

//	edr0        = (wd1   <= wd2  ) && interp_restriction_lv1;
	edr0.x      = (wd1.x <= wd2.x) && interp_restriction_lv1.x;
	edr0.y      = (wd1.y <= wd2.y) && interp_restriction_lv1.y;
	edr0.z      = (wd1.z <= wd2.z) && interp_restriction_lv1.z;
	edr0.w      = (wd1.w <= wd2.w) && interp_restriction_lv1.w;

//	edr         = (wd1   <  wd2  ) && interp_restriction_lv1   && ( !eq(f,b)   && !id(f,c,f,b)   || !eq(h,d)   && !id(h,g,h,d)   || eq(e,g)   || eq(e,c)   );	
	edr.x       = (wd1.x <  wd2.x) && interp_restriction_lv1.x && ( !eq(f,b).x && !id(f,c,f,b).x || !eq(h,d).x && !id(h,g,h,d).x || eq(e,g).x || eq(e,c).x );
	edr.y       = (wd1.y <  wd2.y) && interp_restriction_lv1.y && ( !eq(f,b).y && !id(f,c,f,b).y || !eq(h,d).y && !id(h,g,h,d).y || eq(e,g).y || eq(e,c).y );
	edr.z       = (wd1.z <  wd2.z) && interp_restriction_lv1.z && ( !eq(f,b).z && !id(f,c,f,b).z || !eq(h,d).z && !id(h,g,h,d).z || eq(e,g).z || eq(e,c).z );
	edr.w       = (wd1.w <  wd2.w) && interp_restriction_lv1.w && ( !eq(f,b).w && !id(f,c,f,b).w || !eq(h,d).w && !id(h,g,h,d).w || eq(e,g).w || eq(e,c).w );

//	edr_left    = ((cf2*df(f,g)  )  <= df(h,c)  )  && interp_restriction_lv2_left   && edr;	
	edr_left.x  = ((cf2*df(f,g).x)  <= df(h,c).x)  && interp_restriction_lv2_left.x && edr.x;
	edr_left.y  = ((cf2*df(f,g).y)  <= df(h,c).y)  && interp_restriction_lv2_left.y && edr.y;
	edr_left.z  = ((cf2*df(f,g).z)  <= df(h,c).z)  && interp_restriction_lv2_left.z && edr.z;
	edr_left.w  = ((cf2*df(f,g).w)  <= df(h,c).w)  && interp_restriction_lv2_left.w && edr.w;

//	edr_up      = (df(f,g)    >= (cf2*df(h,c)  ))  && interp_restriction_lv2_up     && edr;	
	edr_up.x    = (df(f,g).x  >= (cf2*df(h,c).x))  && interp_restriction_lv2_up.x   && edr.x;
	edr_up.y    = (df(f,g).y  >= (cf2*df(h,c).y))  && interp_restriction_lv2_up.y   && edr.y;
	edr_up.z    = (df(f,g).z  >= (cf2*df(h,c).z))  && interp_restriction_lv2_up.z   && edr.z;
	edr_up.w    = (df(f,g).w  >= (cf2*df(h,c).w))  && interp_restriction_lv2_up.w   && edr.w;

//	edr3_left   = ((cf3*df(f,g0)  ) <= df(h,c1)  ) && interp_restriction_lv3_left   && edr_left;	
	edr3_left.x = ((cf3*df(f,g0).x) <= df(h,c1).x) && interp_restriction_lv3_left.x && edr_left.x;
	edr3_left.y = ((cf3*df(f,g0).y) <= df(h,c1).y) && interp_restriction_lv3_left.y && edr_left.y;
	edr3_left.w = ((cf3*df(f,g0).w) <= df(h,c1).w) && interp_restriction_lv3_left.w && edr_left.w;
	edr3_left.z = ((cf3*df(f,g0).z) <= df(h,c1).z) && interp_restriction_lv3_left.z && edr_left.z;

//	edr3_up     = (df(f,g0)   >= (cf3*df(h,c1)  )) && interp_restriction_lv3_up     && edr_up;	
	edr3_up.x   = (df(f,g0).x >= (cf3*df(h,c1).x)) && interp_restriction_lv3_up.x   && edr_up.x;
	edr3_up.y   = (df(f,g0).y >= (cf3*df(h,c1).y)) && interp_restriction_lv3_up.y   && edr_up.y;
	edr3_up.w   = (df(f,g0).w >= (cf3*df(h,c1).w)) && interp_restriction_lv3_up.w   && edr_up.w;
	edr3_up.z   = (df(f,g0).z >= (cf3*df(h,c1).z)) && interp_restriction_lv3_up.z   && edr_up.z;

//	edr4_left   = ((cf4*df(f,g2)  ) <= df(h,c3)  ) && interp_restriction_lv4_left   && edr3_left;	
	edr4_left.x = ((cf4*df(f,g2).x) <= df(h,c3).x) && interp_restriction_lv4_left.x && edr3_left.x;
	edr4_left.y = ((cf4*df(f,g2).y) <= df(h,c3).y) && interp_restriction_lv4_left.y && edr3_left.y;
	edr4_left.z = ((cf4*df(f,g2).z) <= df(h,c3).z) && interp_restriction_lv4_left.z && edr3_left.z;
	edr4_left.w = ((cf4*df(f,g2).w) <= df(h,c3).w) && interp_restriction_lv4_left.w && edr3_left.w;

//	edr4_up     = (df(f,g2)   >= (cf4*df(h,c3)  )) && interp_restriction_lv4_up     && edr3_up;		
	edr4_up.x   = (df(f,g2).x >= (cf4*df(h,c3).x)) && interp_restriction_lv4_up.x   && edr3_up.x;
	edr4_up.y   = (df(f,g2).y >= (cf4*df(h,c3).y)) && interp_restriction_lv4_up.y   && edr3_up.y;
	edr4_up.z   = (df(f,g2).z >= (cf4*df(h,c3).z)) && interp_restriction_lv4_up.z   && edr3_up.z;
	edr4_up.w   = (df(f,g2).w >= (cf4*df(h,c3).w)) && interp_restriction_lv4_up.w   && edr3_up.w;

	vec4 info;
//	info   = (edr4_left   && !edr4_up  ) ? float4(8.0) : ((edr4_up   && !edr4_left  ) ? float4(7.0) : ((edr3_left   && !edr3_up  ) ? float4(6.0) : ((edr3_up   && !edr3_left  ) ? float4(5.0) : ((edr_left   && !edr_up  ) ? float4(4.0) : ((edr_up   && !edr_left  ) ? float4(3.0) : (edr   ? float4(2.0) : (edr0   ? float4(1.0) : float4(0.0))))))));
	info.x = (edr4_left.x && !edr4_up.x) ?  float(8.0) : ((edr4_up.x && !edr4_left.x) ?  float(7.0) : ((edr3_left.x && !edr3_up.x) ?  float(6.0) : ((edr3_up.x && !edr3_left.x) ?  float(5.0) : ((edr_left.x && !edr_up.x) ?  float(4.0) : ((edr_up.x && !edr_left.x) ?  float(3.0) : (edr.x ?  float(2.0) : (edr0.x ?  float(1.0) :  float(0.0))))))));
	info.y = (edr4_left.y && !edr4_up.y) ?  float(8.0) : ((edr4_up.y && !edr4_left.y) ?  float(7.0) : ((edr3_left.y && !edr3_up.y) ?  float(6.0) : ((edr3_up.y && !edr3_left.y) ?  float(5.0) : ((edr_left.y && !edr_up.y) ?  float(4.0) : ((edr_up.y && !edr_left.y) ?  float(3.0) : (edr.y ?  float(2.0) : (edr0.y ?  float(1.0) :  float(0.0))))))));
	info.z = (edr4_left.z && !edr4_up.z) ?  float(8.0) : ((edr4_up.z && !edr4_left.z) ?  float(7.0) : ((edr3_left.z && !edr3_up.z) ?  float(6.0) : ((edr3_up.z && !edr3_left.z) ?  float(5.0) : ((edr_left.z && !edr_up.z) ?  float(4.0) : ((edr_up.z && !edr_left.z) ?  float(3.0) : (edr.z ?  float(2.0) : (edr0.z ?  float(1.0) :  float(0.0))))))));
	info.w = (edr4_left.w && !edr4_up.w) ?  float(8.0) : ((edr4_up.w && !edr4_left.w) ?  float(7.0) : ((edr3_left.w && !edr3_up.w) ?  float(6.0) : ((edr3_up.w && !edr3_left.w) ?  float(5.0) : ((edr_left.w && !edr_up.w) ?  float(4.0) : ((edr_up.w && !edr_left.w) ?  float(3.0) : (edr.w ?  float(2.0) : (edr0.w ?  float(1.0) :  float(0.0))))))));

	FragColor = float4(remapTo01(info, maximo));
}

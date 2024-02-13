#version 450

/*
   
  *******  Super XBR Shader - pass0  *******
   
  Copyright (c) 2015 Hyllian - sergiogdb@gmail.com

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
	float XBR_EDGE_STR_P0;
	float XBR_WEIGHT;
	float XBR_ANTI_RINGING;
	float MODE;
	float XBR_EDGE_SHP;
	float XBR_TEXTURE_SHP;
	float SUPERXBR_ON;
} params;

#pragma parameter HSM_SUPER_XBR_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter SUPERXBR_TITLE "[ --- SUPER-XBR by HYLLIAN --- ]:" 0 0 0.01 0.01
#pragma parameter SUPERXBR_ON 		"          Super-XBR ON" 1 0 1 1
#define SUPERXBR_ON params.SUPERXBR_ON

#pragma parameter MODE 				"          Mode - Normal, Details, Adaptive" 1.0 0.0 2.0 1.0
#pragma parameter XBR_EDGE_STR_P0 	"          Xbr - Edge Strength p0" 1.0 0.0 5.0 0.5
#pragma parameter XBR_WEIGHT 		"          Xbr - Filter Weight" 0.0 0.0 1.0 0.1
#pragma parameter XBR_EDGE_SHP 		"          Adaptive Dynamic Edge Sharp" 0.4 0.0 3.0 0.1
#pragma parameter XBR_TEXTURE_SHP 	"          Adaptive Static Edge Sharp" 1.0 0.0 2.0 0.1

#define XBR_EDGE_STR_P0 params.XBR_EDGE_STR_P0
#define XBR_WEIGHT params.XBR_WEIGHT
#define MODE params.MODE
#define XBR_EDGE_SHP params.XBR_EDGE_SHP
#define XBR_TEXTURE_SHP params.XBR_TEXTURE_SHP

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define mul(a,b) (b*a)

const vec3 Y = vec3(.2126, .7152, .0722);

float RGBtoYUV(vec3 color)
{
  return dot(color, Y);
}

float df(float A, float B)
{
  return abs(A-B);
}

/*
                              P1
     |P0|B |C |P1|         C     F4          |a0|b1|c2|d3|
     |D |E |F |F4|      B     F     I4       |b0|c1|d2|e3|   |e1|i1|i2|e2|
     |G |H |I |I4|   P0    E  A  I     P3    |c0|d1|e2|f3|   |e3|i3|i4|e4|
     |P2|H5|I5|P3|      D     H     I5       |d0|e1|f2|g3|
                           G     H5
                              P2
*/


float d_wd(float wp1, float wp2, float wp3, float wp4, float wp5, float wp6, float b0, float b1, float c0, float c1, float c2, float d0, float d1, float d2, float d3, float e1, float e2, float e3, float f2, float f3)
{
	return (wp1*(df(c1,c2) + df(c1,c0) + df(e2,e1) + df(e2,e3)) + wp2*(df(d2,d3) + df(d0,d1)) + wp3*(df(d1,d3) + df(d0,d2)) + wp4*df(d1,d2) + wp5*(df(c0,c2) + df(e1,e3)) + wp6*(df(b0,b1) + df(f2,f3)));
}

float hv_wd(float wp1, float wp2, float wp3, float wp4, float wp5, float wp6, float i1, float i2, float i3, float i4, float e1, float e2, float e3, float e4)
{
	return ( wp4*(df(i1,i2)+df(i3,i4)) + wp1*(df(i1,e1)+df(i2,e2)+df(i3,e3)+df(i4,e4)) + wp3*(df(i1,e2)+df(i3,e4)+df(e1,i2)+df(e3,i4)));
}

vec3 min4(vec3 a, vec3 b, vec3 c, vec3 d)
{
    return min(a, min(b, min(c, d)));
}

vec3 max4(vec3 a, vec3 b, vec3 c, vec3 d)
{
    return max(a, max(b, max(c, d)));
}

float max4float(float a, float b, float c, float d)
{
    return max(a, max(b, max(c, d)));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out vec4 t3;
layout(location = 4) out vec4 t4;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
   
   float dx = params.SourceSize.z;
   float dy = params.SourceSize.w;
   t1 = vTexCoord.xyxy + vec4(-dx, -dy, 2.0*dx, 2.0*dy);
   t2 = vTexCoord.xyxy + vec4(  0, -dy,     dx, 2.0*dy);
   t3 = vTexCoord.xyxy + vec4(-dx,   0, 2.0*dx,     dy);
   t4 = vTexCoord.xyxy + vec4(  0,   0,     dx,     dy);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 4) in vec4 t4;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	if (SUPERXBR_ON < 0.5)
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}

// settings //
	float wp1, wp2, wp3, wp4, wp5, wp6, weight1, weight2;

		wp1	=	2.0;
		wp2	=	1.0;
		wp3	=	-1.0;
		wp4	=	4.0;
		wp5	=	-1.0;
		wp6	=	1.0;

		weight1	=	(XBR_WEIGHT*1.29633/10.0);
		weight2	=	(XBR_WEIGHT*1.75068/10.0/2.0);

// end settings //

	vec3 P0 = texture(Source, t1.xy).xyz;
	vec3 P1 = texture(Source, t1.zy).xyz;
	vec3 P2 = texture(Source, t1.xw).xyz;
	vec3 P3 = texture(Source, t1.zw).xyz;

	vec3  B = texture(Source, t2.xy).xyz;
	vec3  C = texture(Source, t2.zy).xyz;
	vec3 H5 = texture(Source, t2.xw).xyz;
	vec3 I5 = texture(Source, t2.zw).xyz;

	vec3  D = texture(Source, t3.xy).xyz;
	vec3 F4 = texture(Source, t3.zy).xyz;
	vec3  G = texture(Source, t3.xw).xyz;
	vec3 I4 = texture(Source, t3.zw).xyz;

	vec3  E = texture(Source, t4.xy).xyz;
	vec3  F = texture(Source, t4.zy).xyz;
	vec3  H = texture(Source, t4.xw).xyz;
	vec3  I = texture(Source, t4.zw).xyz;

	float b = RGBtoYUV( B );
	float c = RGBtoYUV( C );
	float d = RGBtoYUV( D );
	float e = RGBtoYUV( E );
	float f = RGBtoYUV( F );
	float g = RGBtoYUV( G );
	float h = RGBtoYUV( H );
	float i = RGBtoYUV( I );

	float i4 = RGBtoYUV( I4 ); float p0 = RGBtoYUV( P0 );
	float i5 = RGBtoYUV( I5 ); float p1 = RGBtoYUV( P1 );
	float h5 = RGBtoYUV( H5 ); float p2 = RGBtoYUV( P2 );
	float f4 = RGBtoYUV( F4 ); float p3 = RGBtoYUV( P3 );

	/* Calc edgeness in diagonal directions. */
	float d_edge  = (d_wd( wp1, wp2, wp3, wp4, wp5, wp6, d, b, g, e, c, p2, h, f, p1, h5, i, f4, i5, i4 ) - d_wd( wp1, wp2, wp3, wp4, wp5, wp6, c, f4, b, f, i4, p0, e, i, p3, d, h, i5, g, h5 ));

	/* Calc edgeness in horizontal/vertical directions. */
	float hv_edge = (hv_wd(wp1, wp2, wp3, wp4, wp5, wp6, f, i, e, h, c, i5, b, h5) - hv_wd(wp1, wp2, wp3, wp4, wp5, wp6, e, f, h, i, d, f4, g, i4));

	float limits = XBR_EDGE_STR_P0 + 0.000001;
	float edge_strength = smoothstep(0.0, limits, abs(d_edge));
	
	vec4 w1, w2;
	vec3 c3, c4;
	if (MODE == 2.0)
	{
		float contrast = max(max4float(df(e,f),df(e,i),df(e,h),df(f,h)),max(df(f,i),df(h,i)))/(e+0.001);

		float wgt1 = weight1*(smoothstep(0.0, 0.6, contrast)*XBR_EDGE_SHP + XBR_TEXTURE_SHP);
		float wgt2 = weight2*(smoothstep(0.0, 0.6, contrast)*XBR_EDGE_SHP + XBR_TEXTURE_SHP);
	
		/* Filter weights. Two taps only. */
		w1 = vec4(-wgt1, wgt1+ 0.5, wgt1+ 0.5, -wgt1);
		w2 = vec4(-wgt2, wgt2+0.25, wgt2+0.25, -wgt2);
		c3 = mul(w2, mat4x3(P0+2.0*(D+G)+P2, B+2.0*(E+H)+H5, C+2.0*(F+I)+I5, P1+2.0*(F4+I4)+P3))/3.0;
      c4 = mul(w2, mat4x3(P0+2.0*(C+B)+P1, D+2.0*(F+E)+F4, G+2.0*(I+H)+I4, P2+2.0*(I5+H5)+P3))/3.0;
	}
	else
	{
		/* Filter weights. Two taps only. */
		w1 = vec4(-weight1, weight1+0.5, weight1+0.5, -weight1);
		w2 = vec4(-weight2, weight2+0.25, weight2+0.25, -weight2);
		c3 = mul(w2, mat4x3(D+G, E+H, F+I, F4+I4));
		c4 = mul(w2, mat4x3(C+B, F+E, I+H, I5+H5));
	}

	/* Filtering and normalization in four direction generating four colors. */
    vec3 c1 = mul(w1, mat4x3( P2,   H,   F,   P1 ));
    vec3 c2 = mul(w1, mat4x3( P0,   E,   I,   P3 ));

	/* Smoothly blends the two strongest directions (one in diagonal and the other in vert/horiz direction). */
	vec3 color =  mix(mix(c1, c2, step(0.0, d_edge)), mix(c3, c4, step(0.0, hv_edge)), 1. - edge_strength);

	/* Anti-ringing code. */
	vec3 min_sample = min4( E, F, H, I );
	vec3 max_sample = max4( E, F, H, I );
	color = clamp(color, min_sample, max_sample);

	FragColor = vec4(color, 1.0);
}

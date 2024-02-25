#version 450

/*
   Hyllian's 4xBR v3.8c+ReverseAA (squared) Shader + crt-caligari
   
   Copyright (C) 2011/2012 Hyllian/Jararaca - sergiogdb@gmail.com

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

/*
 *  ReverseAA part of the code
 *
 *  Copyright (c) 2012, Christoph Feck <christoph@maxiom.de>
 *  All Rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float xbr_mode;
} params;

#pragma parameter xbr_mode "xBR Corner Mode (0=Square, 1=Round)" 0.0 0.0 1.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define mul(c,d) (d*c)

const float coef           = 2.0;
const vec4 eq_threshold  = vec4(15.0, 15.0, 15.0, 15.0);
const float y_weight        = 48.0;
const float u_weight        = 7.0;
const float v_weight        = 6.0;
const mat3 yuv          = mat3(0.299, 0.587, 0.114, -0.169, -0.331, 0.499, 0.499, -0.418, -0.0813);
const mat3 yuv_weighted = mat3(y_weight*yuv[0], u_weight*yuv[1], v_weight*yuv[2]);
const vec4 delta       = vec4(0.4, 0.4, 0.4, 0.4);
const float sharpness    = 0.65;

// Constants used with gamma correction.
#define InputGamma 2.4
#define OutputGamma 2.2

#define GAMMA_IN(color)     pow(color, vec3(InputGamma, InputGamma, InputGamma))
#define GAMMA_OUT(color)    pow(color, vec3(1.0 / OutputGamma, 1.0 / OutputGamma, 1.0 / OutputGamma))

#define TEX2D(coords)	GAMMA_IN( texture(Source, coords).xyz )

// 0.5 = the spot stays inside the original pixel
// 1.0 = the spot bleeds up to the center of next pixel
#define SPOT_HEIGHT 0.58

// Used to counteract the desaturation effect of weighting.
#define COLOR_BOOST 1.45

// Macro for weights computing
#define WEIGHT(w) \
	if(w>1.0) w=1.0; \
	w = 1.0 - w * w; \
	w = w * w;\

vec4 df(vec4 A, vec4 B)
{
	return vec4(abs(A-B));
}

float c_df(vec3 c1, vec3 c2)
{
	vec3 df = abs(c1 - c2);
	return df.r + df.g + df.b;
}

bvec4 eq(vec4 A, vec4 B)
{
	vec4 dif = df(A,B);
	return bvec4((dif.x < eq_threshold.x), (dif.y < eq_threshold.y), (dif.z < eq_threshold.z), (dif.w < eq_threshold.w));
}

bvec4 eq2(vec4 A, vec4 B)
{
	vec4 dif = df(A,B);
	return bvec4((dif.x < 2.0), (dif.y < 2.0), (dif.z < 2.0), (dif.w < 2.0));
}

vec4 weighted_distance(vec4 a, vec4 b, vec4 c, vec4 d, vec4 e, vec4 f, vec4 g, vec4 h)
{
	return (df(a,b) + df(a,c) + df(d,e) + df(d,f) + 4.0*df(g,h));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out vec4 t3;
layout(location = 4) out vec4 t4;
layout(location = 5) out vec4 t5;
layout(location = 6) out vec4 t6;
layout(location = 7) out vec4 t7;

void main()
{
   gl_Position = global.MVP * Position;
   texCoord = TexCoord;

	float dx = params.SourceSize.z;
	float dy = params.SourceSize.w;

	//    A1 B1 C1
	// A0  A  B  C C4
	// D0  D  E  F F4
	// G0  G  H  I I4
	//    G5 H5 I5

	// This line fix a bug in ATI cards.
	vec2 texCoord1 = texCoord + vec2(0.0000001, 0.0000001);

	texCoord = texCoord1;
	t1 = texCoord1.xxxy + vec4( -dx, 0, dx,-2.0*dy); // A1 B1 C1
	t2 = texCoord1.xxxy + vec4( -dx, 0, dx,    -dy); //  A  B  C
	t3 = texCoord1.xxxy + vec4( -dx, 0, dx,      0); //  D  E  F
	t4 = texCoord1.xxxy + vec4( -dx, 0, dx,     dy); //  G  H  I
	t5 = texCoord1.xxxy + vec4( -dx, 0, dx, 2.0*dy); // G5 H5 I5
	t6 = texCoord1.xyyy + vec4(-2.0*dx,-dy, 0,  dy); // A0 D0 G0
	t7 = texCoord1.xyyy + vec4( 2.0*dx,-dy, 0,  dy); // C4 F4 I4
}

#pragma stage fragment
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 4) in vec4 t4;
layout(location = 5) in vec4 t5;
layout(location = 6) in vec4 t6;
layout(location = 7) in vec4 t7;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	bvec4 edr, edr_left, edr_up, px; // px = pixel, edr = edge detection rule
	bvec4 interp_restriction_lv1, interp_restriction_lv2_left, interp_restriction_lv2_up;
	bvec4 nc, nc30, nc60, nc45; // new_color
	vec4 fx, fx_left, fx_up, final_fx; // inequations of straight lines.
	vec3 res1, res2, pix1, pix2;
	float blend1, blend2;

	vec2 fp = fract(texCoord*params.SourceSize.xy);

	vec3 A1 = texture(Source, t1.xw).rgb;
	vec3 B1 = texture(Source, t1.yw).rgb;
	vec3 C1 = texture(Source, t1.zw).rgb;

	vec3 A  = texture(Source, t2.xw).rgb;
	vec3 B  = texture(Source, t2.yw).rgb;
	vec3 C  = texture(Source, t2.zw).rgb;

	vec3 D  = texture(Source, t3.xw).rgb;
	vec3 E  = texture(Source, t3.yw).rgb;
	vec3 F  = texture(Source, t3.zw).rgb;

	vec3 G  = texture(Source, t4.xw).rgb;
	vec3 H  = texture(Source, t4.yw).rgb;
	vec3 I  = texture(Source, t4.zw).rgb;

	vec3 G5 = texture(Source, t5.xw).rgb;
	vec3 H5 = texture(Source, t5.yw).rgb;
	vec3 I5 = texture(Source, t5.zw).rgb;

	vec3 A0 = texture(Source, t6.xy).rgb;
	vec3 D0 = texture(Source, t6.xz).rgb;
	vec3 G0 = texture(Source, t6.xw).rgb;

	vec3 C4 = texture(Source, t7.xy).rgb;
	vec3 F4 = texture(Source, t7.xz).rgb;
	vec3 I4 = texture(Source, t7.xw).rgb;

	vec4 b = mul( mat4x3(B, D, H, F), yuv_weighted[0] );
	vec4 c = mul( mat4x3(C, A, G, I), yuv_weighted[0] );
	vec4 e = mul( mat4x3(E, E, E, E), yuv_weighted[0] );
	vec4 a = c.yzwx;
	vec4 d = b.yzwx;
	vec4 f = b.wxyz;
	vec4 g = c.zwxy;
	vec4 h = b.zwxy;
	vec4 i = c.wxyz;

	vec4 i4 = mul( mat4x3(I4, C1, A0, G5), yuv_weighted[0] );
	vec4 i5 = mul( mat4x3(I5, C4, A1, G0), yuv_weighted[0] );
	vec4 h5 = mul( mat4x3(H5, F4, B1, D0), yuv_weighted[0] );
	vec4 f4 = h5.yzwx;


	vec4 Ao = vec4( 1.0, -1.0, -1.0, 1.0 );
	vec4 Bo = vec4( 1.0,  1.0, -1.0,-1.0 );
	vec4 Co = vec4( 1.5,  0.5, -0.5, 0.5 );
	vec4 Ax = vec4( 1.0, -1.0, -1.0, 1.0 );
	vec4 Bx = vec4( 0.5,  2.0, -0.5,-2.0 );
	vec4 Cx = vec4( 1.0,  1.0, -0.5, 0.0 );
	vec4 Ay = vec4( 1.0, -1.0, -1.0, 1.0 );
	vec4 By = vec4( 2.0,  0.5, -2.0,-0.5 );
	vec4 Cy = vec4( 2.0,  0.0, -1.0, 0.5 );

	// These inequations define the line below which interpolation occurs.
	fx      = (Ao*fp.y+Bo*fp.x); 
	fx_left = (Ax*fp.y+Bx*fp.x);
	fx_up   = (Ay*fp.y+By*fp.x);

// unroll this whole crazy thing
	bvec4 e_not_f = notEqual(e,f);
	bvec4 e_not_h = notEqual(e,h);
	bvec4 less2_eb = eq2(e,b);
	bvec4 less2_ed = eq2(e,d);
	bvec4 less2_ea = eq2(e,a);
	bvec4 less2_ff4 = eq2(f,f4);
	bvec4 less2_fc = eq2(f,c);
	bvec4 less2_hh5 = eq2(h,h5);
	bvec4 less2_hg = eq2(h,g);
	bvec4 dfthr_fb = eq(f,b);
	bvec4 dfthr_fc = eq(f,c);
	bvec4 dfthr_hd = eq(h,d);
	bvec4 dfthr_hg = eq(h,g);
	bvec4 dfthr_ei = eq(e,i);
	bvec4 dfthr_ff4 = eq(f,f4);
	bvec4 dfthr_fi4 = eq(f,i4);
	bvec4 dfthr_hh5 = eq(h,h5);
	bvec4 dfthr_hi5 = eq(h,i5);
	bvec4 dfthr_eg = eq(e,g);
	bvec4 dfthr_ec = eq(e,c);

// xBR corner mode B
if (params.xbr_mode < 0.5){
	interp_restriction_lv1.x = ( e_not_f.x && e_not_h.x && ((less2_eb.x || less2_ed.x || !less2_ea.x) && (less2_ff4.x || less2_fc.x || less2_hh5.x || less2_hg.x)) && ( !dfthr_fb.x && !dfthr_fc.x || !dfthr_hd.x && !dfthr_hg.x || dfthr_ei.x && (!dfthr_ff4.x && !dfthr_fi4.x || !dfthr_hh5.x && !dfthr_hi5.x) || dfthr_eg.x || dfthr_ec.x) );

	interp_restriction_lv1.y = ( e_not_f.y && e_not_h.y && ((less2_eb.y || less2_ed.y || !less2_ea.y) && (less2_ff4.y || less2_fc.y || less2_hh5.y || less2_hg.y)) && ( !dfthr_fb.y && !dfthr_fc.y || !dfthr_hd.y && !dfthr_hg.y || dfthr_ei.y && (!dfthr_ff4.y && !dfthr_fi4.y || !dfthr_hh5.y && !dfthr_hi5.y) || dfthr_eg.y || dfthr_ec.y) );

	interp_restriction_lv1.z = ( e_not_f.z && e_not_h.z && ((less2_eb.z || less2_ed.z || !less2_ea.z) && (less2_ff4.z || less2_fc.z || less2_hh5.z || less2_hg.z)) && ( !dfthr_fb.z && !dfthr_fc.z || !dfthr_hd.z && !dfthr_hg.z || dfthr_ei.z && (!dfthr_ff4.z && !dfthr_fi4.z || !dfthr_hh5.z && !dfthr_hi5.z) || dfthr_eg.z || dfthr_ec.z) );

	interp_restriction_lv1.w = ( e_not_f.w && e_not_h.w && ((less2_eb.w || less2_ed.w || !less2_ea.w) && (less2_ff4.w || less2_fc.w || less2_hh5.w || less2_hg.w)) && ( !dfthr_fb.w && !dfthr_fc.w || !dfthr_hd.w && !dfthr_hg.w || dfthr_ei.w && (!dfthr_ff4.w && !dfthr_fi4.w || !dfthr_hh5.w && !dfthr_hi5.w) || dfthr_eg.w || dfthr_ec.w) );
}
// xBR corner mode C
else
{
	interp_restriction_lv1.x = ( e_not_f.x && e_not_h.x && ((less2_eb.x || less2_ed.x || !less2_ea.x) && (less2_ff4.x || less2_fc.x || less2_hh5.x || less2_hg.x)) && ( !dfthr_fb.x && !dfthr_hd.x || dfthr_ei.x && !dfthr_fi4.x && !dfthr_hi5.x || dfthr_eg.x || dfthr_ec.x) );

	interp_restriction_lv1.y = ( e_not_f.y && e_not_h.y && ((less2_eb.y || less2_ed.y || !less2_ea.y) && (less2_ff4.y || less2_fc.y || less2_hh5.y || less2_hg.y)) && ( !dfthr_fb.y && !dfthr_hd.y || dfthr_ei.y && !dfthr_fi4.y && !dfthr_hi5.y || dfthr_eg.y || dfthr_ec.y) );

	interp_restriction_lv1.z = ( e_not_f.z && e_not_h.z && ((less2_eb.z || less2_ed.z || !less2_ea.z) && (less2_ff4.z || less2_fc.z || less2_hh5.z || less2_hg.z)) && ( !dfthr_fb.z && !dfthr_hd.z || dfthr_ei.z && !dfthr_fi4.z && !dfthr_hi5.z || dfthr_eg.z || dfthr_ec.z) );

	interp_restriction_lv1.w = ( e_not_f.w && e_not_h.w && ((less2_eb.w || less2_ed.w || !less2_ea.w) && (less2_ff4.w || less2_fc.w || less2_hh5.w || less2_hg.w)) && ( !dfthr_fb.w && !dfthr_hd.w || dfthr_ei.w && !dfthr_fi4.w && !dfthr_hi5.w || dfthr_eg.w || dfthr_ec.w) );
}

	interp_restriction_lv2_left.x = ((e.x!=g.x) && (d.x!=g.x));
	interp_restriction_lv2_left.y = ((e.y!=g.y) && (d.y!=g.y));
	interp_restriction_lv2_left.z = ((e.z!=g.z) && (d.z!=g.z));
	interp_restriction_lv2_left.w = ((e.w!=g.w) && (d.w!=g.w));

	interp_restriction_lv2_up.x   = ((e.x!=c.x) && (b.x!=c.x));
	interp_restriction_lv2_up.y   = ((e.y!=c.y) && (b.y!=c.y));
	interp_restriction_lv2_up.z   = ((e.z!=c.z) && (b.z!=c.z));
	interp_restriction_lv2_up.w   = ((e.w!=c.w) && (b.w!=c.w));

	vec4 fx45 = smoothstep(Co - delta, Co + delta, fx);
	vec4 fx30 = smoothstep(Cx - delta, Cx + delta, fx_left);
	vec4 fx60 = smoothstep(Cy - delta, Cy + delta, fx_up);

	vec4 wd1 = weighted_distance( e, c, g, i, h5, f4, h, f);
	vec4 wd2 = weighted_distance( h, d, i5, f, i4, b, e, i);

	edr.x      = ((wd1.x + 3.5) < wd2.x) && interp_restriction_lv1.x;
	edr.y      = ((wd1.y + 3.5) < wd2.y) && interp_restriction_lv1.y;
	edr.z      = ((wd1.z + 3.5) < wd2.z) && interp_restriction_lv1.z;
	edr.w      = ((wd1.w + 3.5) < wd2.w) && interp_restriction_lv1.w;

	vec4 diff_fg = df(f,g);
	vec4 diff_hc = df(h,c);

	edr_left.x = ((coef*diff_fg.x) <= diff_hc.x) && interp_restriction_lv2_left.x;
	edr_left.y = ((coef*diff_fg.y) <= diff_hc.y) && interp_restriction_lv2_left.y;
	edr_left.z = ((coef*diff_fg.z) <= diff_hc.z) && interp_restriction_lv2_left.z;
	edr_left.w = ((coef*diff_fg.w) <= diff_hc.w) && interp_restriction_lv2_left.w;

	edr_up.x   = (diff_fg.x >= (coef*diff_hc.x)) && interp_restriction_lv2_up.x;
	edr_up.y   = (diff_fg.y >= (coef*diff_hc.y)) && interp_restriction_lv2_up.y;
	edr_up.z   = (diff_fg.z >= (coef*diff_hc.z)) && interp_restriction_lv2_up.z;
	edr_up.w   = (diff_fg.w >= (coef*diff_hc.w)) && interp_restriction_lv2_up.w;

	nc45.x = ( edr.x &&               bool(fx45.x));
	nc45.y = ( edr.y &&               bool(fx45.y));
	nc45.z = ( edr.z &&               bool(fx45.z));
	nc45.w = ( edr.w &&               bool(fx45.w));

	nc30.x = ( edr.x && edr_left.x && bool(fx30.x));
	nc30.y = ( edr.t && edr_left.y && bool(fx30.y));
	nc30.z = ( edr.z && edr_left.z && bool(fx30.z));
	nc30.w = ( edr.w && edr_left.w && bool(fx30.w));

	nc60.x = ( edr.x && edr_up.x   && bool(fx60.x));
	nc60.y = ( edr.y && edr_up.y   && bool(fx60.y));
	nc60.z = ( edr.z && edr_up.z   && bool(fx60.z));
	nc60.w = ( edr.w && edr_up.w   && bool(fx60.w));

	vec4 diff_ef = df(e,f);
	vec4 diff_eh = df(e,h);

	px.x = (diff_ef.x <= diff_eh.x);
	px.y = (diff_ef.y <= diff_eh.y);
	px.z = (diff_ef.z <= diff_eh.z);
	px.w = (diff_ef.w <= diff_eh.w);

	vec3 res = E;


    vec3 n1, n2, n3, n4, s, aa, bb, cc, dd;


    n1 = B1; n2 = B; s = E; n3 = H; n4 = H5;
    aa = n2-n1; bb = s-n2; cc = n3-s; dd = n4-n3;

    vec3 t = (7. * (bb + cc) - 3. * (aa + dd)) / 16.;

    vec3 m;
	m.x = (s.x < 0.5) ? 2.*s.x : 2.*(1.0-s.x);
	m.y = (s.y < 0.5) ? 2.*s.y : 2.*(1.0-s.y);
	m.z = (s.z < 0.5) ? 2.*s.z : 2.*(1.0-s.z);

        m = min(m, sharpness*abs(bb));
        m = min(m, sharpness*abs(cc));

    t = clamp(t, -m, m);

   
    vec3 s1 = (2*fp.y-1)*t + s;

    n1 = D0; n2 = D; s = s1; n3 = F; n4 = F4;
    aa = n2-n1; bb = s-n2; cc = n3-s; dd = n4-n3;

    t = (7. * (bb + cc) - 3. * (aa + dd)) / 16.;

    m.x = (s.x < 0.5) ? 2.*s.x : 2.*(1.0-s.x);
    m.y = (s.y < 0.5) ? 2.*s.y : 2.*(1.0-s.y);
    m.z = (s.z < 0.5) ? 2.*s.z : 2.*(1.0-s.z);

        m = min(m, sharpness*abs(bb));
        m = min(m, sharpness*abs(cc));

    t = clamp(t, -m, m);

    vec3 s0 = (2.*fp.x-1.)*t + s;


	nc.x = (nc30.x || nc60.x || nc45.x);
	nc.y = (nc30.y || nc60.y || nc45.y);
	nc.z = (nc30.z || nc60.z || nc45.z);
	nc.w = (nc30.w || nc60.w || nc45.w);

	blend1 = blend2 = 0.0;

	vec4 final45 = vec4(dot(vec4(nc45), fx45));
	vec4 final30 = vec4(dot(vec4(nc30), fx30));
	vec4 final60 = vec4(dot(vec4(nc60), fx60));

	vec4 maximo = max(max(final30, final60), final45);

	     if (nc.x) {pix1 = px.x ? F : H; blend1 = maximo.x;}
	else if (nc.y) {pix1 = px.y ? B : F; blend1 = maximo.y;}
	else if (nc.z) {pix1 = px.z ? D : B; blend1 = maximo.z;}
	else if (nc.w) {pix1 = px.w ? H : D; blend1 = maximo.w;}

	     if (nc.w) {pix2 = px.w ? H : D; blend2 = maximo.w;}
	else if (nc.z) {pix2 = px.z ? D : B; blend2 = maximo.z;}
	else if (nc.y) {pix2 = px.y ? B : F; blend2 = maximo.y;}
	else if (nc.x) {pix2 = px.x ? F : H; blend2 = maximo.x;}

	res1 = mix(s0, pix1, blend1);
	res2 = mix(s0, pix2, blend2);

	res = mix(res1, res2, step(c_df(E, res1), c_df(E, res2)));

// CRT-caligari - only vertical blend

            vec3 color = GAMMA_IN(res);

            float ddy = fp.y - 0.5;
            float v_weight_00 = ddy / SPOT_HEIGHT;
            WEIGHT(v_weight_00);
            color *= vec3( v_weight_00, v_weight_00, v_weight_00 );

	    // get closest vertical neighbour to blend
 	    vec3 coords10;
            if (ddy>0.0) {
            	coords10 = H;
                ddy = 1.0 - ddy;
            } else {
                coords10 = B;
                ddy = 1.0 + ddy;
            }
	    vec3 colorNB = GAMMA_IN(coords10);

            float v_weight_10 = ddy / SPOT_HEIGHT;
            WEIGHT( v_weight_10 );

            color += colorNB * vec3( v_weight_10, v_weight_10, v_weight_10 );

            color *= vec3( COLOR_BOOST, COLOR_BOOST, COLOR_BOOST );

   FragColor = vec4(clamp( GAMMA_OUT(color), 0.0, 1.0 ), 1.0);

//   FragColor = vec4(res.x, res.y, res.z, 1.0);
}

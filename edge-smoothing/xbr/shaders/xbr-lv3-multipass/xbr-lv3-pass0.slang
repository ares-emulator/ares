#version 450

/*
   Hyllian's xBR-lv3 - pass0 Shader
   
   Copyright (C) 2011-2022 Hyllian - sergiogdb@gmail.com

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


   Incorporates some of the ideas from SABR shader. Thanks to Joshua Street.
*/

layout(push_constant) uniform Push
{
    float SMALL_DETAILS;
    float WP4;
    float KA;
    float XBR_EQ_THRESHOLD;
    float FD;
} params;

#pragma parameter SMALL_DETAILS       "XBR - MODE (VANILLA | SMALL DETAILS)" 0.0 0.0 1.0 1.0
#pragma parameter WP4                        "XBR - SMOOTHNESS LOCAL WEIGHT" 1.0 0.0 1.0 0.2
#pragma parameter KA                            "XBR - SMOOTHNESS THRESHOLD" 1.0 0.0 1.0 0.05
#pragma parameter XBR_EQ_THRESHOLD       "XBR - COLOR DISTINCTION THRESHOLD" 0.26 0.0 1.0 0.01
#pragma parameter FD "XBR - DITHERING/SMOOTHNESS COMPROMISE (MEDIUM | HIGH)" 0.0 0.0 1.0 1.0

#define SMALL_DETAILS         params.SMALL_DETAILS
#define WP4                   (5.0*params.WP4)
#define KA                    (6.0-5.0*params.KA)
#define XBR_EQ_THRESHOLD      params.XBR_EQ_THRESHOLD
#define FD                    (1.0-params.FD)

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
} global;

// Uncomment just one of the three params below to choose the corner detection
//#define CORNER_A
//#define CORNER_B
#define CORNER_C

#define XBR_EQ_THRESHOLD2 0.02
#define lv2_cf       2.4
#define saturate(c)  clamp(c, 0.0, 1.0)
#define round(X)     floor((X)+0.5)

const vec3 v2f       = vec3( 65536, 256, 1); // vec to float encode
const vec4 zip       = vec4(  1.0,   2.0,   4.0,   8.0);
const vec4 zip2      = vec4( 16.0,  32.0,  64.0, 128.0);
const vec4 low       = vec4(  0.0,   0.0,   0.0,   0.0);
const vec4 high      = vec4(255.0, 255.0, 255.0, 255.0);
const vec3 Y         = vec3(0.2627, 0.6780, 0.0593);
const vec3 Ysd       = vec3( 0.299,  0.587,  0.114);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
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
//   vTexCoord = TexCoord;
   vTexCoord = TexCoord * 1.0001;
   
    float dx = global.SourceSize.z;
    float dy = global.SourceSize.w;

    //    A1 B1 C1
    // A0  A  B  C C4
    // D0  D  E  F F4
    // G0  G  H  I I4
    //    G5 H5 I5

    t1 = vTexCoord.xxxy + vec4( -dx, 0, dx,-2.0*dy); // A1 B1 C1
    t2 = vTexCoord.xxxy + vec4( -dx, 0, dx,    -dy); //  A  B  C
    t3 = vTexCoord.xxxy + vec4( -dx, 0, dx,      0); //  D  E  F
    t4 = vTexCoord.xxxy + vec4( -dx, 0, dx,     dy); //  G  H  I
    t5 = vTexCoord.xxxy + vec4( -dx, 0, dx, 2.0*dy); // G5 H5 I5
    t6 = vTexCoord.xyyy + vec4(-2.0*dx,-dy, 0,  dy); // A0 D0 G0
    t7 = vTexCoord.xyyy + vec4( 2.0*dx,-dy, 0,  dy); // C4 F4 I4
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 4) in vec4 t4;
layout(location = 5) in vec4 t5;
layout(location = 6) in vec4 t6;
layout(location = 7) in vec4 t7;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

vec4 remapTo01(vec4 v, vec4 low, vec4 high)
{
    return saturate((v - low)/(high-low));
}

// Return if A components are less than or equal B ones.
vec4 LTE(vec4 A, vec4 B)
{
    return step(A, B);
}

// Return if A components are less than B ones.
vec4 LT(vec4 A, vec4 B)
{
    return vec4(lessThan(A, B));
}

// Return logically inverted vector components. BEWARE: Only works with 0.0 or 1.0 components.
vec4 NOT(vec4 A)
{
    return (vec4(1.0) - A);
}

// Compare two vectors and return their components are different.
vec4 diff(vec4 A, vec4 B)
{
    return vec4(notEqual(A, B));
}

float dist(vec3 A, vec3 B)
{
    return dot(abs(A-B), Y);
}


// Calculate color distance between two vectors of four pixels
vec4 dist4(mat4x3 A, mat4x3 B)
{
    return vec4(dist(A[0],B[0]), dist(A[1],B[1]), dist(A[2],B[2]), dist(A[3],B[3]));
}

// Tests if color components are under a threshold. In this case they are considered 'equal'.
vec4 eq(mat4x3 A, mat4x3 B)
{
    return (step(dist4(A, B), vec4(XBR_EQ_THRESHOLD)));
}

vec4 eq2(mat4x3 A, mat4x3 B)
{
    return (step(dist4(A, B), vec4(XBR_EQ_THRESHOLD2)));
}

// Determine if two vector components are NOT equal based on a threshold.
vec4 neq(mat4x3 A, mat4x3 B)
{
    return (vec4(1.0, 1.0, 1.0, 1.0) - eq(A, B));
}

// Calculate weighted distance among pixels in some directions.
vec4 weighted_distance(mat4x3 a, mat4x3 b, mat4x3 c, mat4x3 d, mat4x3 e, mat4x3 f, mat4x3 g, mat4x3 h)
{
    if (SMALL_DETAILS < 0.5)
    {
        return ( FD * ( dist4(a,b) + dist4(a,c)) * (dist4(d,e) + dist4(d,f) ) + WP4 * dist4(g,h) );
    }
    else
    {
        return ( FD * ( dist4(a,b) + dist4(a,c) + dist4(d,e) + dist4(d,f) - dist4(c,e) - dist4(b,f) ) + WP4 * dist4(g,h) );
    }
}



void main()
{
    vec4 edri, edr, edr_l, edr_u, edr3_l, edr3_u; // edr = edge detection rule, l = left, u = up.
    vec4 irlv0, irlv1, irlv2l, irlv2u, irlv3l, irlv3u; // ir = interpolation restriction

    vec3 A1   = texture(Source, t1.xw).rgb;
    vec3 B1   = texture(Source, t1.yw).rgb;
    vec3 C1   = texture(Source, t1.zw).rgb;

    vec3 A    = texture(Source, t2.xw).rgb;
    vec3 B    = texture(Source, t2.yw).rgb;
    vec3 C    = texture(Source, t2.zw).rgb;

    vec3 D    = texture(Source, t3.xw).rgb;
    vec3 E    = texture(Source, t3.yw).rgb;
    vec3 F    = texture(Source, t3.zw).rgb;

    vec3 G    = texture(Source, t4.xw).rgb;
    vec3 H    = texture(Source, t4.yw).rgb;
    vec3 I    = texture(Source, t4.zw).rgb;

    vec3 G5   = texture(Source, t5.xw).rgb;
    vec3 H5   = texture(Source, t5.yw).rgb;
    vec3 I5   = texture(Source, t5.zw).rgb;

    vec3 A0   = texture(Source, t6.xy).rgb;
    vec3 D0   = texture(Source, t6.xz).rgb;
    vec3 G0   = texture(Source, t6.xw).rgb;

    vec3 C4   = texture(Source, t7.xy).rgb;
    vec3 F4   = texture(Source, t7.xz).rgb;
    vec3 I4   = texture(Source, t7.xw).rgb;

    mat4x3 b  = mat4x3(B, D, H, F);
    mat4x3 c  = mat4x3(C, A, G, I);
    mat4x3 d  = mat4x3(D, H, F, B);
    mat4x3 e  = mat4x3(E, E, E, E);
    mat4x3 f  = mat4x3(F, B, D, H);
    mat4x3 g  = mat4x3(G, I, C, A);
    mat4x3 h  = mat4x3(H, F, B, D);
    mat4x3 i  = mat4x3(I, C, A, G);

    mat4x3 i4 = mat4x3(I4, C1, A0, G5);
    mat4x3 i5 = mat4x3(I5, C4, A1, G0);
    mat4x3 h5 = mat4x3(H5, F4, B1, D0);
    mat4x3 f4 = mat4x3(F4, B1, D0, H5);

    mat4x3 c4 = mat4x3(C4, A1, G0, I5);
    mat4x3 g5 = mat4x3(G5, I4, C1, A0);

    mat4x3 c1 = mat4x3(C1, A0, G5, I4);
    mat4x3 g0 = mat4x3(G0, I5, C4, A1);
    mat4x3 b1 = mat4x3(B1, D0, H5, F4);
    mat4x3 d0 = mat4x3(D0, H5, F4, B1);

    vec4 b_   = v2f * b;
    vec4 c_   = v2f * c;
    vec4 d_   = b_.yzwx;
    vec4 e_   = v2f * e;
    vec4 f_   = b_.wxyz;
    vec4 g_   = c_.zwxy;
    vec4 h_   = b_.zwxy;
    vec4 i_   = c_.wxyz;

    vec4 i4_  = v2f * i4;
    vec4 i5_  = v2f * i5;
    vec4 h5_  = v2f * h5;
    vec4 f4_  = h5_.yzwx;

    vec4 c4_  = i5_.yzwx;
    vec4 g5_  = i4_.wxyz;

    vec4 c1_  = i4_.yzwx;
    vec4 g0_  = i5_.wxyz;
    vec4 b1_  = h5_.zwxy;
    vec4 d0_  = h5_.wxyz;

    irlv0     = diff(e_, f_) * diff(e_, h_);
    irlv1     = irlv0;

#ifdef CORNER_B
    irlv1      = saturate(irlv0 * ( neq(f,b) * neq(h,d) + eq(e,i) * neq(f,i4) * neq(h,i5) + eq(e,g) + eq(e,c) ) );
#endif
#ifdef CORNER_C
    irlv1     = saturate(irlv0  * ( neq(f,b) * neq(f,c) + neq(h,d) * neq(h,g) + eq(e,i) * (neq(f,f4) * neq(f,i4) + neq(h,h5) * neq(h,i5)) + eq(e,g) + eq(e,c)) );
#endif

    irlv2l    = diff(e_, g_) * diff(d_, g_);
    irlv2u    = diff(e_, c_) * diff(b_, c_);
    irlv3l = eq2(g,g0) * diff(d0_,g0_)  * eq(e,c4);
    irlv3u = eq2(c,c1) * diff(b1_,c1_)  * eq(e,g5);

    vec4 wd1  = weighted_distance( e, c, g, i, h5, f4, h, f );
    vec4 wd2  = weighted_distance( h, d, i5, f, i4, b, e, i );

    vec4 d_fg = dist4(f, g);
    vec4 d_hc = dist4(h, c);

    edri      = LTE(wd1, wd2) * irlv0;
    edr       = LT( KA*wd1, wd2) * irlv1 * NOT(edri.yzwx * edri.wxyz);
    edr_l     = LTE( lv2_cf * d_fg, d_hc ) * irlv2l * edr * (NOT(edri.yzwx) * eq(e, c));
    edr_u     = LTE( lv2_cf * d_hc, d_fg ) * irlv2u * edr * (NOT(edri.wxyz) * eq(e, g));
    edr3_l    = irlv3l * edr_l;
    edr3_u    = irlv3u * edr_u;

    vec4 px = LTE(dist4(e,f), dist4(e,h));

    vec4 info  = zip  * mat4(   edr,  edr_l, edr_u,      edri);
         info += zip2 * mat4(edr3_l, edr3_u,    px, vec4(0.0));

    FragColor = remapTo01(info, low, high);
}

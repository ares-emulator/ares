#version 450

/*
   Hyllian's xBR-lv1 Shader

   Copyright (C) 2011-2022 Hyllian - sergiogdb@gmail.com

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to permit
   persons to whom the Software is furnished to do so, subject to the
   following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   Incorporates some of the ideas from SABR shader. Thanks to Joshua Street.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float XBR_EQ_THRESHOLD;
	float XBR_BLENDING;
} params;

#pragma parameter XBR_EQ_THRESHOLD "COLOR DISTINCTION THRESHOLD" 0.32 0.0 1.0 0.01
#pragma parameter XBR_BLENDING "BLENDING [ NOBLEND | AA ]" 1.0 0.0 1.0 1.0

#define XBR_EQ_THRESHOLD params.XBR_EQ_THRESHOLD
#define XBR_BLENDING params.XBR_BLENDING

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// Uncomment just one of the three params below to choose the corner detection
//#define CORNER_A
//#define CORNER_B
#define CORNER_C

#define saturate(c) clamp(c, 0.0, 1.0)

const  vec4 Ao = vec4( 1.0, -1.0, -1.0, 1.0 );
const  vec4 Bo = vec4( 1.0,  1.0, -1.0,-1.0 );
const  vec4 Co = vec4( 1.5,  0.5, -0.5, 0.5 );
const  vec4 Ci = vec4(0.25, 0.25, 0.25, 0.25);

const vec3 v2f = vec3( 65536, 256, 1); // vec to float encode
const vec3 Y = vec3(0.2627, 0.6780, 0.0593);

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

// Determine if two vector components are NOT equal based on a threshold.
vec4 neq(mat4x3 A, mat4x3 B)
{
    return (vec4(1.0, 1.0, 1.0, 1.0) - eq(A, B));
}

// Calculate weighted distance among pixels in some directions.
vec4 weighted_distance(mat4x3 a, mat4x3 b, mat4x3 c, mat4x3 d, mat4x3 e, mat4x3 f, mat4x3 g, mat4x3 h)
{
    return (dist4(a,b) + dist4(a,c) + dist4(d,e) + dist4(d,f) + 4.0*dist4(g,h));
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
layout(location = 8) out float aa_factor;

void main()
{
   gl_Position = global.MVP * Position;
   texCoord = TexCoord * 1.0001;

   aa_factor = 2.0*params.OutputSize.z * params.SourceSize.x;

   float dx = params.SourceSize.z;
   float dy = params.SourceSize.w;

   //    A1 B1 C1
   // A0  A  B  C C4
   // D0  D  E  F F4
   // G0  G  H  I I4
   //    G5 H5 I5

   t1 = texCoord.xxxy + vec4( -dx, 0, dx,-2.0*dy); // A1 B1 C1
   t2 = texCoord.xxxy + vec4( -dx, 0, dx,    -dy); //  A  B  C
   t3 = texCoord.xxxy + vec4( -dx, 0, dx,      0); //  D  E  F
   t4 = texCoord.xxxy + vec4( -dx, 0, dx,     dy); //  G  H  I
   t5 = texCoord.xxxy + vec4( -dx, 0, dx, 2.0*dy); // G5 H5 I5
   t6 = texCoord.xyyy + vec4(-2.0*dx,-dy, 0,  dy); // A0 D0 G0
   t7 = texCoord.xyyy + vec4( 2.0*dx,-dy, 0,  dy); // C4 F4 I4
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
layout(location = 8) in float aa_factor;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec4 edri, edr, px; // px = pixel, edr = edge detection rule
    vec4 irlv0, irlv1;
    vec4 fx; // inequations of straight lines.
    vec3 res1, res2;
    vec4 fx45i, fx45;

    vec2 fp  = fract(texCoord*params.SourceSize.xy);

    vec3 A1 = texture(Source, t1.xw ).xyz;
    vec3 B1 = texture(Source, t1.yw ).xyz;
    vec3 C1 = texture(Source, t1.zw ).xyz;
    vec3 A  = texture(Source, t2.xw ).xyz;
    vec3 B  = texture(Source, t2.yw ).xyz;
    vec3 C  = texture(Source, t2.zw ).xyz;
    vec3 D  = texture(Source, t3.xw ).xyz;
    vec3 E  = texture(Source, t3.yw ).xyz;
    vec3 F  = texture(Source, t3.zw ).xyz;
    vec3 G  = texture(Source, t4.xw ).xyz;
    vec3 H  = texture(Source, t4.yw ).xyz;
    vec3 I  = texture(Source, t4.zw ).xyz;
    vec3 G5 = texture(Source, t5.xw ).xyz;
    vec3 H5 = texture(Source, t5.yw ).xyz;
    vec3 I5 = texture(Source, t5.zw ).xyz;
    vec3 A0 = texture(Source, t6.xy ).xyz;
    vec3 D0 = texture(Source, t6.xz ).xyz;
    vec3 G0 = texture(Source, t6.xw ).xyz;
    vec3 C4 = texture(Source, t7.xy ).xyz;
    vec3 F4 = texture(Source, t7.xz ).xyz;
    vec3 I4 = texture(Source, t7.xw ).xyz;

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

    // These inequations define the line below which interpolation occurs.
    fx    = ( Ao*fp.y + Bo*fp.x );

    irlv0 = diff(e_,f_) * diff(e_,h_);
    irlv1 = irlv0;

#ifdef CORNER_B
    irlv1      = saturate(irlv0 * ( neq(f,b) * neq(h,d) + eq(e,i) * neq(f,i4) * neq(h,i5) + eq(e,g) + eq(e,c) ) );
#endif
#ifdef CORNER_C
    irlv1     = saturate(irlv0  * ( neq(f,b) * neq(f,c) + neq(h,d) * neq(h,g) + eq(e,i) * (neq(f,f4) * neq(f,i4) + neq(h,h5) * neq(h,i5)) + eq(e,g) + eq(e,c)) );
#endif

    if (XBR_BLENDING == 1.0) {
        vec4 delta  = vec4(aa_factor);

        fx45i = saturate( 0.5 + (fx   - Co - Ci) / delta  );
        fx45  = saturate( 0.5 + (fx   - Co     ) / delta  );
    }
    else {
        fx45i = LT( Co + Ci, fx   );
        fx45  = LT(      Co, fx   );
    }
       
    vec4 wd1 = weighted_distance( e, c,  g, i, h5, f4, h, f);
    vec4 wd2 = weighted_distance( h, d, i5, f, i4,  b, e, i);

    edri      = LTE(wd1, wd2) * irlv0;
    edr       = LT( wd1, wd2) * irlv1 * NOT(edri.yzwx * edri.wxyz);

    fx45i = edri   * fx45i;
    fx45  = edr    * fx45;

    px = LTE(dist4(e,f), dist4(e,h));

    vec4 maximos = max(fx45, fx45i);

    res1 = mix(E, mix(H, F, px.x), maximos.x);
    res2 = mix(E, mix(B, D, px.z), maximos.z);

    vec3 res1a = mix(res1, res2, step(dist(E, res1), dist(E, res2)));

    res1 = mix(E, mix(F, B, px.y), maximos.y);
    res2 = mix(E, mix(D, H, px.w), maximos.w);

    vec3 res1b = mix(res1, res2, step(dist(E, res1), dist(E, res2)));

    vec3 res = mix(res1a, res1b, step(dist(E, res1a), dist(E, res1b)));

    FragColor = vec4(res, 1.0);
}

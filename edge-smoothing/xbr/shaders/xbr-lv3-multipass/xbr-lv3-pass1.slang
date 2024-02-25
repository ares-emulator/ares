#version 450

/*
   Hyllian's xBR-lv3 - pass1 Shader
   
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
*/

layout(push_constant) uniform Push
{
	float XBR_BLENDING;
} params;

#pragma parameter XBR_BLENDING "BLENDING [ NOBLEND | AA ]" 1.0 0.0 1.0 1.0

#define XBR_BLENDING params.XBR_BLENDING

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
} global;

#define saturate(c)  clamp(c, 0.0, 1.0)
#define round(X)     floor((X)+0.5)

const vec4 low       = vec4(  0.0,   0.0,   0.0,   0.0);
const vec4 high      = vec4(255.0, 255.0, 255.0, 255.0);
const vec3 Y = vec3(0.2627, 0.6780, 0.0593);

const  vec4 Ao = vec4( 1.0, -1.0, -1.0, 1.0 );
const  vec4 Bo = vec4( 1.0,  1.0, -1.0,-1.0 );
const  vec4 Co = vec4( 1.5,  0.5, -0.5, 0.5 );
const  vec4 Ax = vec4( 1.0, -1.0, -1.0, 1.0 );
const  vec4 Bx = vec4( 0.5,  2.0, -0.5,-2.0 );
const  vec4 Cx = vec4( 1.0,  1.0, -0.5, 0.0 );
const  vec4 Ay = vec4( 1.0, -1.0, -1.0, 1.0 );
const  vec4 By = vec4( 2.0,  0.5, -2.0,-0.5 );
const  vec4 Cy = vec4( 2.0,  0.0, -1.0, 0.5 );
const  vec4 Ci = vec4(0.25, 0.25, 0.25, 0.25);

const  vec4 Az = vec4( 6.0, -2.0, -6.0, 2.0 );
const  vec4 Bz = vec4( 2.0, 6.0, -2.0, -6.0 );
const  vec4 Cz = vec4( 5.0, 3.0, -3.0, -1.0 );
const  vec4 Aw = vec4( 2.0, -6.0, -2.0, 6.0 );
const  vec4 Bw = vec4( 6.0, 2.0, -6.0,-2.0 );
const  vec4 Cw = vec4( 5.0, -1.0, -3.0, 3.0 );

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out float aa_factor;

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

    aa_factor = 2.0*global.OutputSize.z * global.SourceSize.x;

    t1 = vTexCoord.xyyy + vec4(   0, -dy,  0, dy);  //  B
    t2 = vTexCoord.xxxy + vec4( -dx,   0, dx,  0);  //D E F
                                                    //  H
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in float aa_factor;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D XbrSource;

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

float dist(vec3 A, vec3 B)
{
    return dot(abs(A-B), Y);
}

// Calculate color distance between two vectors of four pixels
vec4 dist4(mat4x3 A, mat4x3 B)
{
    return vec4(dist(A[0],B[0]), dist(A[1],B[1]), dist(A[2],B[2]), dist(A[3],B[3]));
}

vec4 remapFrom01(vec4 v, vec4 low, vec4 high)
{
    return round(mix(low, high, v));
}

vec4 unpack_info(float i)
{
    vec4 info;
    info.x = round(modf(i/2.0, i));
    info.y = round(modf(i/2.0, i));
    info.z = round(modf(i/2.0, i));
    info.w = i;

    return info;
}

/*
mat4 unpack_info(vec4 i)
{
    mat4 info;
    info[0] = round(modf(i/2.0, i));
    info[1] = round(modf(i/2.0, i));
    info[2] = round(modf(i/2.0, i));
    info[3] = i;

    return transpose(info);
}*/

mat2x4 unpack_info2(float i)
{
    mat2x4 info;
    info[0].x = round(modf(i/2.0, i));
    info[0].y = round(modf(i/2.0, i));
    info[0].z = round(modf(i/2.0, i));
    info[0].w = round(modf(i/2.0, i));
    info[1].x = round(modf(i/2.0, i));
    info[1].y = round(modf(i/2.0, i));
    info[1].z = round(modf(i/2.0, i));
    info[1].w = i;

    return info;
}

void main()
{
    vec4 edri, edr, edr_l, edr_u, edr3_l, edr3_u, px; // px = pixel, edr = edge detection rule
    vec4 fx, fx_l, fx_u, fx3_l, fx3_u; // inequations of straight lines.
    vec3 res1, res2;
    vec4 fx45i, fx45, fx30, fx60, fx15, fx75;

    vec2 fp = fract(vTexCoord*global.SourceSize.xy);

    vec3 B  = texture(XbrSource, t1.xy).rgb;
    vec3 D  = texture(XbrSource, t2.xw).rgb;
    vec3 E  = texture(XbrSource, t2.yw).rgb;
    vec3 F  = texture(XbrSource, t2.zw).rgb;
    vec3 H  = texture(XbrSource, t1.xw).rgb;

    mat4x3 e  = mat4x3(E, E, E, E);
    mat4x3 f  = mat4x3(F, B, D, H);
    mat4x3 h  = mat4x3(H, F, B, D);

    // These inequations define the line below which interpolation occurs.
    fx   = (Ao*fp.y+Bo*fp.x); 
    fx_l = (Ax*fp.y+Bx*fp.x);
    fx_u = (Ay*fp.y+By*fp.x);
    fx3_l = ( Az*fp.y + Bz*fp.x );
    fx3_u = ( Aw*fp.y + Bw*fp.x );

    if (XBR_BLENDING == 1.0) {
        vec4 delta  = vec4(aa_factor);
        vec4 deltaL = vec4(0.5, 1.0, 0.5, 1.0) * aa_factor;
        vec4 deltaU = deltaL.yxwz;

        fx45  = saturate( 0.5 + (fx    - Co     ) / delta  );
        fx30  = saturate( 0.5 + (fx_l  - Cx     ) / deltaL );
        fx60  = saturate( 0.5 + (fx_u  - Cy     ) / deltaU );
        fx45i = saturate( 0.5 + (fx    - Co - Ci) / delta  );
        fx15  = saturate( 0.5 + (fx3_l - Cz     ) / deltaL );
        fx75  = saturate( 0.5 + (fx3_u - Cw     ) / deltaU );
    }
    else {
        fx45  = LT(      Co, fx   );
        fx30  = LT(      Cx, fx_l );
        fx60  = LT(      Cy, fx_u );
        fx45i = LT( Co + Ci, fx   );
        fx15  = LT(      Cz, fx3_l );
        fx75  = LT(      Cw, fx3_u );
    }

    vec4 info  = texture(Source, vTexCoord);

    vec4 i = remapFrom01(info, low, high);

    mat2x4 edx = unpack_info2(i.x);
    mat2x4 edy = unpack_info2(i.y);
    mat2x4 edz = unpack_info2(i.z);

    edr    = edx[0];
    edr_l  = edy[0];
    edr_u  = edz[0];
    edri   = unpack_info(i.w);
    edr3_l = edx[1];
    edr3_u = edy[1];
    px     = edz[1];

    fx45i = edri   * fx45i;
    fx45  = edr    * fx45;
    fx30  = edr_l  * fx30;
    fx60  = edr_u  * fx60;
    fx15  = edr3_l * fx15;
    fx75  = edr3_u * fx75;

    vec4 maximos = max(max(max(fx15, fx30), max(fx60, fx75)), max(fx45, fx45i));

    res1 = mix(E, mix(H, F, px.x), maximos.x);
    res2 = mix(E, mix(B, D, px.z), maximos.z);

    vec3 res1a = mix(res1, res2, step(dist(E, res1), dist(E, res2)));

    res1 = mix(E, mix(F, B, px.y), maximos.y);
    res2 = mix(E, mix(D, H, px.w), maximos.w);

    vec3 res1b = mix(res1, res2, step(dist(E, res1), dist(E, res2)));

    vec3 res = mix(res1a, res1b, step(dist(E, res1a), dist(E, res1b)));


    FragColor = vec4(res, 1.0);
}

#version 450

/*
   Hyllian's DDT-xBR-lv1 Shader
   
   Copyright (C) 2011-2022 Hyllian/Jararaca - sergiogdb@gmail.com

   Sharpness control - Copyright (c) 2022 Filippo Scognamiglio

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
    float USE_DYNAMIC_SHARPNESS;
    float USE_SHARPENING_BIAS;
    float DYNAMIC_SHARPNESS_MIN;
    float DYNAMIC_SHARPNESS_MAX;
    float STATIC_SHARPNESS;
    float DDT_THRESHOLD;
} params;
// set to 1.0 to use dynamic sharpening
#pragma parameter USE_DYNAMIC_SHARPNESS "Dynamic Sharpness [ 0FF | ON ]" 1.0 0.0 1.0 1.0

// Set to 1.0 to bias the interpolation towards sharpening
#pragma parameter USE_SHARPENING_BIAS "Sharpness Bias [ 0FF | ON ]" 1.0 0.0 1.0 1.0

// Minimum amount of sharpening in range [0.0, 1.0]
#pragma parameter DYNAMIC_SHARPNESS_MIN "Dynamic Sharpness Min" 0.0 0.0 1.0 0.1

// Maximum amount of sharpening in range [0.0, 1.0]
#pragma parameter DYNAMIC_SHARPNESS_MAX "Dynamic Sharpness Max" 0.3 0.0 1.0 0.1

// If USE_DYNAMIC_SHARPNESS is 0 apply this static sharpness
#pragma parameter STATIC_SHARPNESS "Static Sharpness" 0.5 0.0 1.0 0.1

#pragma parameter DDT_THRESHOLD "DDT Diagonal Threshold" 2.6 1.0 6.0 0.2

#define USE_DYNAMIC_SHARPNESS  params.USE_DYNAMIC_SHARPNESS
#define USE_SHARPENING_BIAS    params.USE_SHARPENING_BIAS
#define DYNAMIC_SHARPNESS_MIN  (params.DYNAMIC_SHARPNESS_MIN * 0.5)
#define DYNAMIC_SHARPNESS_MAX  (params.DYNAMIC_SHARPNESS_MAX * 0.5)
#define STATIC_SHARPNESS       (params.STATIC_SHARPNESS * 0.5)
#define DDT_THRESHOLD params.DDT_THRESHOLD

#define WP1  4.0
#define WP2  1.0
#define WP3 -1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#define saturate(c) clamp(c, 0.0, 1.0)
#define lerp(a,b,c) mix(a,b,c)
#define mul(a,b) (b*a)
#define fmod(c,d) mod(c,d)
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

#define decal Source

const vec3 Y = vec3( 0.299,  0.587,  0.114);

float luma(float3 color)
{
  return dot(color, Y);
}

float linearStep(float edge0, float edge1, float t)
{
    return clamp((t - edge0) / (edge1 - edge0), 0.0, 1.0);
}

float sharpSmooth(float t, float sharpness)
{
    return linearStep(sharpness, 1.0 - sharpness, t);
}

vec3 quadBilinear(vec3 a, vec3 b, vec3 c, vec3 d, vec2 p, float sharpness)
{
    float x = sharpSmooth(p.x, sharpness);
    float y = sharpSmooth(p.y, sharpness);
    return mix(mix(a, b, x), mix(c, d, x), y);
}

// Fast computation of barycentric coordinates only in the sub-triangle 1 2 4
vec3 fastBarycentric(vec2 p, float sharpness)
{
    float l0 = sharpSmooth(1.0 - p.x - p.y, sharpness);
    float l1 = sharpSmooth(p.x, sharpness);
    return vec3(l0, l1, 1.0 - l0 - l1);
}

vec3 triangleInterpolate(vec3 t1, vec3 t2, vec3 t3, vec3 t4, vec2 c, float sharpness)
{
    // Alter colors and coordinates to compute the other triangle.
    bool altTriangle = 1.0 - c.x < c.y;
    vec3 cornerColor = altTriangle ? t3 : t1;
    vec2 triangleCoords = altTriangle ? vec2(1.0 - c.y, 1.0 - c.x) : c;
    vec3 weights = fastBarycentric(triangleCoords, sharpness);
    return weights.x * cornerColor + weights.y * t2 + weights.z * t4;
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec2 loc;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord * 1.0001;
   
    float2 ps = float2(params.SourceSize.z, params.SourceSize.w);
    float dx = ps.x;
    float dy = ps.y;

    t1.xy = float2( dx,  0); // F
    t1.zw = float2(  0, dy); // H
    loc = vTexCoord*params.SourceSize.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec2 loc;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    float2 pos = frac(loc)-float2(0.5, 0.5); // pos = pixel position
    float2 dir = sign(pos); // dir = pixel direction
    float lmax, lmin, contrast;
    float sharpness = STATIC_SHARPNESS;

    float2 g1 = dir*t1.xy;
    float2 g2 = dir*t1.zw;

    float3 A = tex2D(decal, vTexCoord       ).xyz;
    float3 B = tex2D(decal, vTexCoord +g1   ).xyz;
    float3 C = tex2D(decal, vTexCoord    +g2).xyz;
    float3 D = tex2D(decal, vTexCoord +g1+g2).xyz;

    float3 A1 = tex2D(decal, vTexCoord    -g2).xyz;
    float3 B1 = tex2D(decal, vTexCoord +g1-g2).xyz;
    float3 A0 = tex2D(decal, vTexCoord -g1   ).xyz;
    float3 C0 = tex2D(decal, vTexCoord -g1+g2).xyz;

    float3 B2 = tex2D(decal, vTexCoord +2*g1     ).xyz;
    float3 D2 = tex2D(decal, vTexCoord +2*g1+  g2).xyz;
    float3 C3 = tex2D(decal, vTexCoord      +2*g2).xyz;
    float3 D3 = tex2D(decal, vTexCoord   +g1+2*g2).xyz;

    float a = luma(A);
    float b = luma(B);
    float c = luma(C);
    float d = luma(D);

    if (USE_DYNAMIC_SHARPNESS == 1.0)
    {
        lmax = max(max(a, b), max(c, d));
        lmin = min(min(a, b), min(c, d));
        contrast = (lmax - lmin) / (lmax + lmin + 0.05);

        if (USE_SHARPENING_BIAS == 1.0)
            contrast = sqrt(contrast);

        sharpness = mix(DYNAMIC_SHARPNESS_MIN, DYNAMIC_SHARPNESS_MAX, contrast);
    }	
    
    float a1 = luma(A1);
    float b1 = luma(B1);
    float a0 = luma(A0);
    float c0 = luma(C0);

    float b2 = luma(B2);
    float d2 = luma(D2);
    float c3 = luma(C3);
    float d3 = luma(D3);

    float p = abs(pos.x);
    float q = abs(pos.y);

//    A1 B1
// A0 A  B  B2
// C0 C  D  D2
//    C3 D3

    float wd1 = (WP1*abs(a-d) + WP2*(abs(b-a1) + abs(b-d2) + abs(c-a0) + abs(c-d3)) + WP3*(abs(a1-d2) + abs(a0-d3)));
    float wd2 = (WP1*abs(b-c) + WP2*(abs(a-b1) + abs(a-c0) + abs(d-b2) + abs(d-c3)) + WP3*(abs(b1-c0) + abs(b2-c3)));

    float irlv1 = (abs(a-b)+abs(a-c)+abs(d-c)+abs(d-b));

    vec3 color;

    if ( ((wd1+0.1*DDT_THRESHOLD)*DDT_THRESHOLD < wd2) && (irlv1 > 0.0) )
    {
           color = triangleInterpolate(B, D, C, A, vec2(q, 1-p), sharpness);
    }
    else if ( (wd1 > (wd2+0.1*DDT_THRESHOLD)*DDT_THRESHOLD) && (irlv1 > 0.0))
    {
           color = triangleInterpolate(A, B, D, C, vec2(p, q), sharpness);
    }
    else
        color = quadBilinear(A, B, C, D, vec2(p, q), sharpness);

   FragColor = vec4(color, 1.0);
}

#version 450

/*
   NEDI Hybrid Shader  -  pass2

// This file is a part of MPDN Extensions.
// https://github.com/zachsaw/MPDN_Extensions
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3.0 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library.
// 

   Sources ported from this discussion thread:

      http://forum.doom9.org/showthread.php?t=170727
   
   Ported by Hyllian - 2024.
*/

layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float NEDI_E;
    float FALLBACK_FILTER;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter NEDI_E "EDGE STRENGTH" 0.60 0.0 1.0 0.05
#pragma parameter FALLBACK_FILTER "FALLBACK FILTER (smooth <--> sharp)" 0.0 0.0 3.0 1.0

#define NEDI_E      (1.0-params.NEDI_E)
#define FALLBACK_FILTER      params.FALLBACK_FILTER

#define saturate(c) clamp(c, 0.0, 1.0)
#define lerp(c) mix(c)
#define mul(a,b) (b*a)
#define fmod(c) mod(c)
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
#define float2x3 mat2x3
#define float3x3 mat3x3
#define float4x4 mat4x4
#define float4x2 mat4x2

#define s0 Source

#define NEDI_WEIGHT 2.0
#define NEDI_N 24.0
//#define NEDI_E 0.0

#define ITERATIONS  4
#define WGT   NEDI_WEIGHT

#define width  (params.SourceSize.x)
#define height (params.SourceSize.y)

#define px (0.5 * (params.SourceSize.z))
#define py (0.5 * (params.SourceSize.w))

#define offset 0.0

#define Value(xy) (tex2D(s0,tex+float2(px,py)*(xy)).rgb)//-float4(0,0.5,0.5,0))

#define Get(xy) (dot(Value(xy),float3(.2126, .7152, .0722))+offset)
#define Get4(xy) (float2(Get(xy+WGT*dir[0])+Get(xy+WGT*dir[1]),Get(xy+WGT*dir[2])+Get(xy+WGT*dir[3])))

#define sqr(x) (dot(x,x))
#define I (float2x2(1,0,0,1))

//Conjugate residual 
float2 solve(float2x2 A,float2 b) {
    float2 x = vec2(1.0/4.0);
    float2 r = b - mul(A,x);
    float2 p = r;
    float2 Ar = mul(A,r);
    float2 Ap = Ar;
    for (int k = 0;k < 2; k++){
        float a = min(100.,dot(r,Ar)/dot(Ap,Ap));
        x = x + a*p;
        float2 rk = r; float2 Ark = Ar;
        r = r - a*Ap;
        Ar = mul(A,r);
        float b = dot(r,Ar)/dot(rk,Ark);
        p = r + b*p;
        Ap = Ar + b*Ap;
    }
    return x;
}

//Cramer's method
float2 solvex(float2x2 A,float2 b) { return float2(determinant(float2x2(b,A[1])),determinant(float2x2(A[0],b)))/determinant(A); }

vec4 get_fallback_filter_coeffs()
{
    vec4 filter_coeffs = vec4(0.343443361963,0.0122478272965,0.0122478272965,-0.000297611455683); // B-spline

    if (FALLBACK_FILTER == 1.0) filter_coeffs = vec4(0.6863468418,0.0494245166214,0.0494245166214,-0.0847248039069); // Jinc2
    else if (FALLBACK_FILTER == 2.0) filter_coeffs = vec4(0.650413534366,0.0223491780099,0.0223491780099,-0.0585122208892); // Jinc2 Sharp
    else if (FALLBACK_FILTER == 3.0) filter_coeffs = vec4(0.5625,-0.0459071979683,-0.0459071979683,-0.0625); // Catmull-Rom

    return filter_coeffs;
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
        float2 tex = vTexCoord - float2(0.5,0.5)/params.SourceSize.xy;

/*
                                         wind[1]              wind[2]
-3                                                                 
-2             dir        wind[0]            2                 1 
-1              4           4          4                             4  
 0            1   2       1   2                   
 1              3           3                   3           3       
 2                                        1                       2
 3                                                                     
*/
/*
                                         wind[1]              wind[2]
-3                                            3                1   
-2             dir        wind[0] 
-1            1   4        1   3      1                                3
 0                                     
 1            3   2        2   4                  4        2    
 2
 3                                        2                        4   
*/

    //Define window and directions
   vec2 dir[4] = vec2[4](vec2(-1,-1),vec2(1,1),vec2(-1,1),vec2(1,-1));
   mat4x2 wind1 = mat4x2(vec2(-1,-1),vec2(-1,1),vec2(1,-1),vec2(1,1));
   mat4x2 wind2 = mat4x2(vec2(-3,-1),vec2(-1,3),vec2(1,-3),vec2(3,1));
   mat4x2 wind3 = mat4x2(vec2(-1,-3),vec2(-3,1),vec2(3,-1),vec2(1,3));
   mat4x2 wind4 = mat4x2(vec2(-3,-3),vec2(-3,3),vec2(3,-3),vec2(3,3));


    mat4x2 wind[4] = mat4x2[4](wind1, wind2, wind3, wind4);

    //Initialization
    float2x2 R = float2x2(0.,0.,0.,0.);
    float2 r = float2(0.,0.);
    float3 d = float3(0.,0.,0.);

    //Define weights
    vec4 fb_filter = get_fallback_filter_coeffs();
    fb_filter /= dot(fb_filter,vec4(4.0));
    float w[4] = float[4](4.0, 1.0, 1.0, 1.0);

    //Calculate (local) autocorrelation coefficients
    for (int k = 0; k<ITERATIONS; k+= 1){
        float4 y = float4(Get(wind[k][0]),Get(wind[k][1]),Get(wind[k][2]),Get(wind[k][3]));
        float4x2 C = float4x2(Get4(wind[k][0]),Get4(wind[k][1]),Get4(wind[k][2]),Get4(wind[k][3]));
        R += mul(transpose(C),w[k]*C);
        r += mul(y,w[k]*C);
        d += fb_filter[k]*(Value(wind[k][0])+Value(wind[k][1])+Value(wind[k][2])+Value(wind[k][3]));
    }

    //Normalize
    float n = NEDI_N;
    R /= n; r /= n;

    //Calculate a =  R^-1 . r
    float e = NEDI_E;
    float2 a = solve(R+e*e*I,r+e*e/2.0);

    //Nomalize 'a' (prevents overshoot)
    a = .25 + float2(.5,-.5)*clamp(a[0]-a[1],-1.0,1.0);

    float3 col0 = Value(dir[0]);
    float3 col1 = Value(dir[1]);
    float3 col2 = Value(dir[2]);
    float3 col3 = Value(dir[3]);

    float3 min_sample = min( min(col0, col1), min(col2, col3) );
    float3 max_sample = max( max(col0, col1), max(col2, col3) );

    // Allows 5% ringing. Needs more tests.
    d = clamp(d, 0.95*min_sample, 1.05*max_sample);

    //Calculate result
    //float2x3 x = float2x3(Value(dir[0])+Value(dir[1]),Value(dir[2])+Value(dir[3]));
    float2x3 x = float2x3(col0 + col1, col2 + col3);
    float3 c = mul(a,x);

    //Fallback to lanczos
    float t = saturate(1.0-500.0*sqr(x[0]-x[1])); // Diagonal local edge strength
    c = mix(c, d, smoothstep(0.0, 1.0, t));
    
    FragColor = float4(c, 1.0);//+float4(0,0.5,0.5,0);
}

#version 450

layout(push_constant) uniform Push
{
    float g_csize;
    float g_bsize;
    float g_flicker;
    float g_shaker;
    float g_refltog;
    float g_reflstr;
    float g_reflgrain;
    float g_fresnel;
    float g_reflblur;
    float gz;
    float gx;
    float gy;
    float gzr;
    float gzg;
    float gzb;
    float goxr;
    float goyr;
    float goxg;
    float goyg;
    float goxb;
    float goyb;
    float TO;
    float PH;
    float PER;
    float ASAT;
}params;


/*
   CRT Glass shader
      > CRT related artifacts:
        ::grain
        ::glass inner reflection
        ::glass outer reflection
        ::chromatic aberration (for beam deconvergence and glass diffraction)
        ::screen flicker
        ::screen jitter
        ::afterglow (dr-venom's mod)
        ::CRT border corner (cgwg's crt-geom)
      > Stack just before scanlines. Works better with curved geometry modes.

   Author: Dogway
   License: Public domain
*/

#pragma parameter g_csize      "Corner Size"         0.02 0.0 0.07 0.01
#pragma parameter g_bsize      "Border Smoothness"   600.0 100.0 600.0 25.0
#pragma parameter g_flicker    "Screen Flicker"      0.25 0.0 1.0 0.01
#pragma parameter g_shaker     "Screen Shake"        0.02 0.0 0.5 0.01
#pragma parameter g_refltog    "Reflection Toggle"   1.0 0.0 1.0 1.00
#pragma parameter g_reflgrain  "Refl. Deband Grain"  0.0 0.0 2.0 0.01
#pragma parameter g_reflstr    "Refl. Brightness"    0.15 0.0 1.0 0.01
#pragma parameter g_fresnel    "Refl. Fresnel"       1.0 0.0 1.0 0.10
#pragma parameter g_reflblur   "Refl. Blur"          0.6 0.0 1.0 0.10
#pragma parameter gz           "Zoom"                1.2 1.0 1.5 0.01
#pragma parameter gx           "Shift-X"             0.0 -1.0 1.0 0.01
#pragma parameter gy           "Shift-Y"            -0.01 -1.0 1.0 0.01
#pragma parameter gzr          "Zoom Red"            1.03 1.0 1.5 0.01
#pragma parameter gzg          "Zoom Green"          1.01 1.0 1.5 0.01
#pragma parameter gzb          "Zoom Blue"           1.0 1.0 1.5 0.01
#pragma parameter goxr         "Shift-X Red"         0.0 -1.0 1.0 0.01
#pragma parameter goyr         "Shift-Y Red"        -0.01 -1.0 1.0 0.01
#pragma parameter goxg         "Shift-X Green"       0.0 -1.0 1.0 0.01
#pragma parameter goyg         "Shift-Y Green"      -0.01 -1.0 1.0 0.01
#pragma parameter goxb         "Shift-X Blue"        0.0 -1.0 1.0 0.01
#pragma parameter goyb         "Shift-Y Blue"        0.0 -1.0 1.0 0.01

// https://www.desmos.com/calculator/1nfq4uubnx
// PER = 2.0 for realistic (1.0 or less when using scanlines). Phosphor Index; it's the same as in the "grade" shader
#pragma parameter TO           "Afterglow OFF/ON"                            1.0 0.0 1.0 1.0
#pragma parameter PH           "AG Phosphor (0:RGB 1:NTSC-U 2:NTSC-J 3:PAL)" 2.0 0.0 3.0 1.0
#pragma parameter ASAT         "Afterglow Saturation"                        0.20 0.0 1.0 0.01
#pragma parameter PER          "Persistence (more is less)"                  0.75 0.5 2.0 0.1


#define SW    params.TO
#define PH    params.PH
#define sat   params.ASAT
#define PER   params.PER
#define GRAIN params.g_reflgrain

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    uint FrameCount;
#include "../../include/img/param_floats.h"
} global;

#include "../../include/img/helper_macros.h"
#include "../../include/img/white_point.h"
#define temperature global.temperature

#define reflblur params.g_reflblur

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out vec4 t3;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    float blur = abs(1. - reflblur) + 1.;
    float dx = global.SourceSize.z / blur;
    float dy = global.SourceSize.w / blur;

    t1 = vTexCoord.xxxy + vec4( -dx,  0.0,  dx, -dy);
    t2 = vTexCoord.xxxy + vec4( -dx,  0.0,  dx, 0.0);
    t3 = vTexCoord.xxxy + vec4( -dx,  0.0,  dx,  dy);
}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D OriginalHistory2;
layout(set = 0, binding = 5) uniform sampler2D OriginalHistory3;
layout(set = 0, binding = 6) uniform sampler2D OriginalHistory4;
layout(set = 0, binding = 7) uniform sampler2D OriginalHistory5;
layout(set = 0, binding = 8) uniform sampler2D OriginalHistory6;

#define Prev1Texture OriginalHistory1
#define Prev2Texture OriginalHistory2
#define Prev3Texture OriginalHistory3
#define Prev4Texture OriginalHistory4
#define Prev5Texture OriginalHistory5
#define Prev6Texture OriginalHistory6
#define TEX0 vTexCoord



// Wide usage friendly PRNG, shamelessly stolen from a GLSL tricks forum post.
// Obtain random numbers by calling rand(h), followed by h = permute(h) to
// update the state. Assumes the texture was hooked.
float mod289(float x)
{
    return x - floor(x / 289.0) * 289.0;
}

float permute(float x)
{
    return mod289((34.0 * x + 1.0) * x);
}

float randg(float x)
{
    return fract(x * 0.024390243);
}


float rand(float co, float size){
    return fract(sin(dot(co, 12.9898)) * size);
}


vec3 afterglow(float Pho, vec3 decay)
{
    // Rec.601
    vec3 RGB =    vec3(0.299, 0.587, 0.114);
    // SMPTE
    vec3 NTSC =   vec3(0.310, 0.595, 0.095);
    // JAP
    vec3 NTSC_J = vec3(0.280, 0.605, 0.115);
    // PAL
    vec3 PAL =    vec3(0.290, 0.600, 0.110);

    vec3 p_in;

    if (Pho ==  0.0) { p_in = RGB;            } else
    if (Pho ==  1.0) { p_in = NTSC;           } else
    if (Pho ==  2.0) { p_in = NTSC_J;         } else
    if (Pho ==  3.0) { p_in = PAL;            }

// Phosphor Response / Cone Response
    vec3 p_res = (p_in / (vec3(0.21259990334510803, 0.71517896652221680, 0.07222118973731995)) / 10.0);

    float decr = clamp((log(1. / p_res.r) + 0.2) / (decay.r), 0., 1.);
    float decg = clamp((log(1. / p_res.g) + 0.2) / (decay.g), 0., 1.);
    float decb = clamp((log(1. / p_res.b) + 0.2) / (decay.b), 0., 1.);

    return vec3(decr, decg, decb);
}

//  Borrowed from cgwg's crt-geom, under GPL
float corner(vec2 coord)
{
    coord *= global.OriginalSize.xy / global.SourceSize.xy;
    coord = (coord - vec2(0.5)) * 1.0 + vec2(0.5);
    coord = min(coord, vec2(1.0)-coord) * vec2(1.0, global.OutputSize.y/global.OutputSize.x);
    vec2 cdist = vec2(max(params.g_csize, max((1.0-smoothstep(100.0,600.0,params.g_bsize))*0.01,0.002)));
    coord = (cdist - min(coord,cdist));
    float dist = sqrt(dot(coord,coord));
    return clamp((cdist.x-dist)*params.g_bsize,0.0, 1.0);
}




void main()
{


    vec2 c_dist = (vec2(0.5) * global.SourceSize.xy) / global.OutputSize.xy;
    vec2 ch_dist = (global.SourceSize.xy / global.OutputSize.xy) / 2.;
    vec2 vpos = vTexCoord * (global.OutputSize.xy / global.SourceSize.xy);

    float vert_msk = abs(1. - vpos.y);
    float center_msk = clamp(abs(1. - (vTexCoord.x) * global.SourceSize.x / global.OutputSize.x - ch_dist.x), 0., 1.);
    float horiz_msk = clamp(max(center_msk - 0.2, 0.0) + 0.1, 0., 1.);

    float zoom   = fract(params.gz) / 10.;


// Screen Jitter ------------------------------------

    float scale   = 2.0 + params.g_shaker / 0.05;
    float prob    = 0.5 + params.g_shaker / 3.0;
    float shaker  = rand(float(global.FrameCount), 43758.5453)                * \
                    rand(float(global.FrameCount), 4.37585453) * params.g_shaker;

          shaker  = shaker + shaker * round(rand(float(global.FrameCount), 53.7585453) * prob) * scale * clamp(params.g_shaker, 0., 0.01) * 100.;

    vec2 coords  = vec2(params.gx,   params.gy   + shaker * 0.5);
    vec2 coordsr = vec2(params.goxr, params.goyr + shaker);
    vec2 coordsg = vec2(params.goxg, params.goyg + shaker);
    vec2 coordsb = vec2(params.goxb, params.goyb + shaker);


// Screen Zoom ------------------------------------

    float cr = texture(Source, (vTexCoord.xy - c_dist) / (fract(params.gzr)/20. + 1.) + c_dist + coordsr/40.).r;
    float cg = texture(Source, (vTexCoord.xy - c_dist) / (fract(params.gzg)/20. + 1.) + c_dist + coordsg/40.).g;
    float cb = texture(Source, (vTexCoord.xy - c_dist) / (fract(params.gzb)/20. + 1.) + c_dist + coordsb/40.).b;

    vec3 color = vec3(cr,cg,cb);

// AfterGlow --------------------------------------

    vec3 color1 = texture(Prev1Texture, TEX0.xy).rgb * afterglow(PH, vec3(PER) * 10.);
    vec3 color2 = texture(Prev2Texture, TEX0.xy).rgb * afterglow(PH, vec3(PER) * 20.);
    vec3 color3 = texture(Prev3Texture, TEX0.xy).rgb * afterglow(PH, vec3(PER) * 30.);
    vec3 color4 = texture(Prev4Texture, TEX0.xy).rgb * afterglow(PH, vec3(PER) * 40.);
    vec3 color5 = texture(Prev5Texture, TEX0.xy).rgb * afterglow(PH, vec3(PER) * 50.);
    vec3 color6 = texture(Prev6Texture, TEX0.xy).rgb * afterglow(PH, vec3(PER) * 60.);

    vec3 glow = max(max(max(max(max(color1, color2), color3), color4), color5), color6);

    glow = normalize(pow(glow + vec3(0.001), vec3(sat)))*length(glow);

    float glowY  = dot(pow(glow,  vec3(2.2)), vec3(0.21260, 0.71518, 0.07222));
    float colorY = dot(pow(color, vec3(2.2)), vec3(0.21260, 0.71518, 0.07222));

    vec3 colormax = (colorY > glowY) ? color : glow;

    color = (SW == 0.0) ? color : clamp(colormax,0.0,1.0);

//--------------------------------------

    float rA = texture(Source, (t1.xw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;
    float rB = texture(Source, (t1.yw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;
    float rC = texture(Source, (t1.zw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;
    float rD = texture(Source, (t2.xw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;
    float rE = texture(Source, (t2.yw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;
    float rF = texture(Source, (t2.zw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;
    float rG = texture(Source, (t3.xw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;
    float rH = texture(Source, (t3.yw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;
    float rI = texture(Source, (t3.zw - c_dist) / (fract(params.gzr)/10. + zoom + 1.) + c_dist + (coordsr + coords)/20.).x;

    float gA = texture(Source, (t1.xw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;
    float gB = texture(Source, (t1.yw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;
    float gC = texture(Source, (t1.zw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;
    float gD = texture(Source, (t2.xw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;
    float gE = texture(Source, (t2.yw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;
    float gF = texture(Source, (t2.zw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;
    float gG = texture(Source, (t3.xw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;
    float gH = texture(Source, (t3.yw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;
    float gI = texture(Source, (t3.zw - c_dist) / (fract(params.gzg)/10. + zoom + 1.) + c_dist + (coordsg + coords)/20.).y;

    float bA = texture(Source, (t1.xw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;
    float bB = texture(Source, (t1.yw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;
    float bC = texture(Source, (t1.zw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;
    float bD = texture(Source, (t2.xw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;
    float bE = texture(Source, (t2.yw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;
    float bF = texture(Source, (t2.zw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;
    float bG = texture(Source, (t3.xw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;
    float bH = texture(Source, (t3.yw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;
    float bI = texture(Source, (t3.zw - c_dist) / (fract(params.gzb)/10. + zoom + 1.) + c_dist + (coordsb + coords)/20.).z;


    vec3 sumA = vec3(rA, gA, bA);
    vec3 sumB = vec3(rB, gB, bB);
    vec3 sumC = vec3(rC, gC, bC);
    vec3 sumD = vec3(rD, gD, bD);
    vec3 sumE = vec3(rE, gE, bE);
    vec3 sumF = vec3(rF, gF, bF);
    vec3 sumG = vec3(rG, gG, bG);
    vec3 sumH = vec3(rH, gH, bH);
    vec3 sumI = vec3(rI, gI, bI);

    vec3 blurred = (sumE+sumA+sumC+sumD+sumF+sumG+sumI+sumB+sumH) / 9.0;


    vpos *= 1. - vpos.xy;
    float vig = vpos.x * vpos.y * 10.;
    float vig_msk = abs(1. - vig) * (center_msk * 2. + 0.3);
    vig = abs(1. - pow(vig, 0.1)) * vert_msk * (center_msk * 2. + 0.3);

    blurred = min((vig_msk + (1. - params.g_fresnel)), 1.0) * blurred;
    vig = clamp(vig * params.g_fresnel, 0.001, 1.0);
    vec3 vig_c = white_point(vec3(vig));

// Reflection in
    vec4 reflection = clamp(vec4((1. - (1. - color ) * (1. - blurred.rgb * params.g_reflstr)) / (1. + params.g_reflstr / 3.), 1.), 0., 1.);


// Reflection-out noise dithering, from deband.slang
    // Initialize the PRNG by hashing the position + a random uniform
    vec3 m = vec3(vTexCoord, randg(sin(vTexCoord.x / vTexCoord.y) * mod(global.FrameCount, 79) + 22.759)) + vec3(1.);
    float h = permute(permute(permute(m.x) + m.y) + m.z);

    if (GRAIN > 0.0)
        {
            vec3 noise;
            noise.x = randg(h); h = permute(h);
            noise.y = randg(h); h = permute(h);
            noise.z = randg(h); h = permute(h);
            vig_c.rgb += GRAIN * (noise - vec3(0.5));
        }

// Reflection out
    reflection = clamp(vec4(1. - (1. - reflection.rgb ) * (1. - vec3(vig_c / 7.)), 1.), 0., 1.);

// Corner Size
    vpos *= (global.SourceSize.xy/global.OutputSize.xy);

// Screen Flicker
    float flicker = (params.g_flicker == 0.0) ? 1.0 : mix(1. - params.g_flicker / 10., 1.0, rand(float(global.FrameCount), 4.37585453));

    reflection = (params.g_refltog == 0.0) ? clamp(texture(Source, vTexCoord.xy) * flicker, 0., 1.) : clamp(reflection * flicker, 0., 1.);
    FragColor = corner(vpos) * reflection;
}

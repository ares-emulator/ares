#version 450

/*
    Hyllian's CRT-sinc Shader

    Copyright (C) 2011-2024 Hyllian - sergiogdb@gmail.com

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
    float HFILTER_PROFILE;
    float SHARPNESS_HACK;
    float SHP;
    float RADIUS;
    float CRT_ANTI_RINGING;
    float CRT_InputGamma;
    float CURVATURE;
    float WARP_X;
    float WARP_Y;
} params;

#pragma parameter HFILTER_PROFILE "H-FILTER [ Custom | Composite1 | Composite2 ]" 0.0 0.0 2.0 1.0
#pragma parameter SHP "  CUSTOM H-FILTER SHARPNESS" 1.0 0.50 1.0 0.01
#pragma parameter RADIUS "  CUSTOM H-FILTER RADIUS" 4.0 2.0 4.0 0.1
#pragma parameter CRT_ANTI_RINGING "ANTI RINGING" 1.0 0.0 1.0 0.1
#pragma parameter SHARPNESS_HACK "SHARPNESS HACK" 1.0 1.0 4.0 1.0
#pragma parameter CRT_InputGamma "INPUT GAMMA" 2.4 1.0 5.0 0.1
#pragma parameter CRT_OutputGamma "OUTPUT GAMMA" 2.2 1.0 5.0 0.05
#pragma parameter PHOSPHOR_LAYOUT "PHOSPHOR LAYOUT [1-6 Aperture, 7-10 Shadow, 11-14 Slot]" 1.0 0.0 15.0 1.0
#pragma parameter MASK_INTENSITY "MASK INTENSITY" 0.65 0.0 1.0 0.01
#pragma parameter MONITOR_SUBPIXELS "MONITOR SUBPIXELS LAYOUT [0=RGB, 1=BGR]" 0.0 0.0 1.0 1.0
#pragma parameter BRIGHTBOOST "BRIGHTNESS BOOST" 1.40 1.0 3.0 0.05
#pragma parameter BEAM_MIN_WIDTH "MIN BEAM WIDTH" 0.86 0.0 1.0 0.02
#pragma parameter BEAM_MAX_WIDTH "MAX BEAM WIDTH" 1.0 0.0 1.0 0.02
#pragma parameter SCANLINES_STRENGTH "SCANLINES STRENGTH" 0.72 0.0 1.0 0.01
#pragma parameter SCANLINES_SHAPE "SCANLINES SHAPE (SINC | GAUSSIAN)" 0.0 0.0 1.0 1.0
#pragma parameter POST_BRIGHTNESS "POST-BRIGHTNESS" 1.00 1.0 3.0 0.05
#pragma parameter CURVATURE "ENABLE CURVATURE" 0.0 0.0 1.0 1.0
#pragma parameter WARP_X "  CURVATURE-X" 0.015 0.0 0.125 0.005
#pragma parameter WARP_Y "  CURVATURE-Y" 0.015 0.0 0.125 0.005
#pragma parameter CORNER_SIZE "  CORNER SIZE" 0.02 0.001 1.0 0.005
#pragma parameter CORNER_SMOOTHNESS "  CORNER SMOOTHNESS" 1.10 1.0 2.2 0.02

#define HFILTER_PROFILE params.HFILTER_PROFILE
#define SHP params.SHP
#define RADIUS params.RADIUS
#define CRT_ANTI_RINGING params.CRT_ANTI_RINGING
#define SHARPNESS_HACK params.SHARPNESS_HACK
#define CRT_InputGamma params.CRT_InputGamma
#define CURVATURE params.CURVATURE
#define WARP_X params.WARP_X
#define WARP_Y params.WARP_Y

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
} global;


#define GAMMA_IN(color)    pow(color, vec3(CRT_InputGamma, CRT_InputGamma, CRT_InputGamma))

#define pi    3.1415926535897932384626433832795

#define AR_MOD  3.0


/* Curvature code. Credits to torridgristle! */
vec2 CRT_Distortion = vec2(WARP_X, 0.0) * 15.;

#define SQRT_OF_2  1.4142135623730950488016887242097

// Radius of Convergence = 1.0 - SQRT_OF_2 / 2

#define CONVERGENCE_RADIUS 0.29289321881345247559915563789515

vec2 Warp(vec2 texCoord)
{
   vec2 cCoords = texCoord * 2.0 - 1.0;
   float cCoordsDist = sqrt(cCoords.x * cCoords.x + cCoords.y * cCoords.y);
   cCoords = cCoords / cCoordsDist;
   cCoords = cCoords * (1.0 - pow(vec2(1.0 - (cCoordsDist/SQRT_OF_2)),(1.0/(1.0+CRT_Distortion*0.2))));
   cCoords = cCoords / (1.0-pow(vec2(CONVERGENCE_RADIUS),(1.0/(vec2(1.0)+CRT_Distortion*0.2))));
   cCoords = cCoords * 0.5 + 0.5;

   return cCoords;
}


vec2 get_hfilter_profile()
{
    vec2 hf_profile = vec2(SHP, RADIUS);

    if      (HFILTER_PROFILE == 1.0) hf_profile = vec2(0.86, 4.0);
    else if (HFILTER_PROFILE == 2.0) hf_profile = vec2(0.75, 4.0);

    return hf_profile;
}

/* Some window functions for tests. */
vec4 sinc(vec4 x)              { return sin(pi*x)/(pi*x+0.00001); }
vec4 hann_window(vec4 x)       { return 0.5 * ( 1.0 - cos( 0.5 * pi * ( x + 2.0 ) ) ); }
vec4 blackman_window(vec4 x)   { return 0.42 - 0.5*cos(0.5*pi*(x+2.0)) + 0.08*cos(pi*(x+2.0)); }
vec4 lanczos(vec4 x, float a)  { return sinc(x) * sinc(x / a); }
vec4 blackman(vec4 x, float a) { return sinc(x) * blackman_window(x); }
vec4 hann(vec4 x, float a)     { return sinc(x) * hann_window(x); }

vec4 resampler4(vec4 x, vec2 hfp)
{
    return lanczos(x * hfp.x, hfp.y);
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 FragCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec3 color;

    vec2 texture_size = vec2(SHARPNESS_HACK*global.SourceSize.x, global.SourceSize.y);

    vec2 dx = vec2(1.0/texture_size.x, 0.0);

    vec2 WarpedTexCoord = vTexCoord.xy;

    WarpedTexCoord = (CURVATURE > 0.5) ? Warp(WarpedTexCoord) : WarpedTexCoord;

    vec2 pix_coord = WarpedTexCoord.xy*texture_size + vec2(-0.5, 0.0);

    vec2 tc = (floor(pix_coord) + vec2(0.5,0.5))/texture_size;

    vec2 fp = fract(pix_coord);

    vec3 c0 = GAMMA_IN(texture(Source, tc - 3.0*dx).xyz);
    vec3 c1 = GAMMA_IN(texture(Source, tc - 2.0*dx).xyz);
    vec3 c2 = GAMMA_IN(texture(Source, tc     - dx).xyz);
    vec3 c3 = GAMMA_IN(texture(Source, tc         ).xyz);
    vec3 c4 = GAMMA_IN(texture(Source, tc     + dx).xyz);
    vec3 c5 = GAMMA_IN(texture(Source, tc + 2.0*dx).xyz);
    vec3 c6 = GAMMA_IN(texture(Source, tc + 3.0*dx).xyz);
    vec3 c7 = GAMMA_IN(texture(Source, tc + 4.0*dx).xyz);

    // Anti-ringing pre filtering
    vec3 min_sample = min(min(c2,c3),min(c4,c5));
    vec3 max_sample = max(max(c2,c3),max(c4,c5));

    vec3 mins = (1.0/AR_MOD) * min_sample;
    vec3 maxs =       AR_MOD * max_sample;

    c0 = clamp(c0, mins, maxs);
    c1 = clamp(c1, mins, maxs);
    c6 = clamp(c6, mins, maxs);
    c7 = clamp(c7, mins, maxs);

    mat4x3 color_matrix0 = mat4x3(c0, c1, c2, c3);
    mat4x3 color_matrix1 = mat4x3(c4, c5, c6, c7);

    vec2 hfp = get_hfilter_profile();

    vec4 wgt0 = resampler4(vec4(3.0+fp.x, 2.0+fp.x, 1.0+fp.x,     fp.x), hfp);
    vec4 wgt1 = resampler4(vec4(1.0-fp.x, 2.0-fp.x, 3.0-fp.x, 4.0-fp.x), hfp);

    float wgt_sum = (dot(wgt0, vec4(1.0))+dot(wgt1, vec4(1.0)));

    wgt0 /= wgt_sum;
    wgt1 /= wgt_sum;

    color  = color_matrix0 * wgt0 + color_matrix1 * wgt1;

    // Anti-ringing post filtering
    color = mix(color, clamp(color, min_sample, max_sample), CRT_ANTI_RINGING);

    FragColor = vec4(color, 1.0);
}

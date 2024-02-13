#version 450

layout(push_constant) uniform Push
{
    float BEAM_PROFILE;
    float HFILTER_PROFILE;
    float BEAM_MIN_WIDTH;
    float BEAM_MAX_WIDTH;
    float SCANLINES_STRENGTH;
    float COLOR_BOOST;
    float SHARPNESS_HACK;
    float PHOSPHOR_LAYOUT;
    float MASK_INTENSITY;
    float CRT_ANTI_RINGING;
    float InputGamma;
    float OutputGamma;
    float VSCANLINES;
} param;

#pragma parameter CRT_HYLLIAN "[CRT-HYLLIAN PARAMS]" 0.0 0.0 0.0 0.0
#pragma parameter BEAM_PROFILE "  BEAM PROFILE (BP)" 0.0 0.0 2.0 1.0
#pragma parameter HFILTER_PROFILE "  HORIZONTAL FILTER PROFILE [ HERMITE | CATMULL-ROM ]" 0.0 0.0 1.0 1.0
#pragma parameter BEAM_MIN_WIDTH "    Custom [If   BP=0.00] MIN BEAM WIDTH" 1.0 0.0 1.0 0.01
#pragma parameter BEAM_MAX_WIDTH "    Custom [If   BP=0.00] MAX BEAM WIDTH" 1.0 0.0 1.0 0.01
#pragma parameter SCANLINES_STRENGTH "    Custom [If   BP=0.00] SCANLINES STRENGTH" 0.58 0.0 1.0 0.01
#pragma parameter COLOR_BOOST "    Custom [If   BP=0.00] COLOR BOOST" 1.30 1.0 2.0 0.05
#pragma parameter SHARPNESS_HACK "  SHARPNESS_HACK" 1.0 1.0 4.0 1.0
#pragma parameter PHOSPHOR_LAYOUT "  PHOSPHOR LAYOUT" 4.0 0.0 24.0 1.0
#pragma parameter MASK_INTENSITY "  MASK INTENSITY" 0.7 0.0 1.0 0.1
#pragma parameter CRT_ANTI_RINGING "  ANTI RINGING" 1.0 0.0 1.0 0.2
#pragma parameter InputGamma "  INPUT GAMMA" 2.4 0.0 5.0 0.1
#pragma parameter OutputGamma "  OUTPUT GAMMA" 2.2 0.0 5.0 0.1
#pragma parameter VSCANLINES "  VERTICAL SCANLINES [ OFF | ON ]" 0.0 0.0 1.0 1.0


layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 FragCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

/*
    Hyllian's CRT Shader

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

#include "../../../include/subpixel_masks.h"

#define GAMMA_IN(color)     pow(color, vec3(param.InputGamma, param.InputGamma, param.InputGamma))
#define GAMMA_OUT(color)    pow(color, vec3(1.0 / param.OutputGamma, 1.0 / param.OutputGamma, 1.0 / param.OutputGamma))




// Horizontal cubic filter.
// Some known filters use these values:

//    B = 0.0, C = 0.0  =>  Hermite cubic filter.
//    B = 1.0, C = 0.0  =>  Cubic B-Spline filter.
//    B = 0.0, C = 0.5  =>  Catmull-Rom Spline filter. This is the default used in this shader.
//    B = C = 1.0/3.0   =>  Mitchell-Netravali cubic filter.
//    B = 0.3782, C = 0.3109  =>  Robidoux filter.
//    B = 0.2620, C = 0.3690  =>  Robidoux Sharp filter.

// Using only Hermite and Catmull-Rom, as the others aren't useful for crt shader.
// For more info, see: http://www.imagemagick.org/Usage/img_diagrams/cubic_survey.gif
mat4x4 get_hfilter_profile()
{
	float bf = 0.0;
	float cf = 0.0;

	if (param.HFILTER_PROFILE == 1) {bf = 0.0;     cf = 0.5;}

        return mat4x4(            (-bf - 6.0*cf)/6.0,         (3.0*bf + 12.0*cf)/6.0,     (-3.0*bf - 6.0*cf)/6.0,             bf/6.0,
                                        (12.0 - 9.0*bf - 6.0*cf)/6.0, (-18.0 + 12.0*bf + 6.0*cf)/6.0,                      0.0, (6.0 - 2.0*bf)/6.0,
                                       -(12.0 - 9.0*bf - 6.0*cf)/6.0, (18.0 - 15.0*bf - 12.0*cf)/6.0,      (3.0*bf + 6.0*cf)/6.0,             bf/6.0,
                                                   (bf + 6.0*cf)/6.0,                           -cf,                      0.0,               0.0);
         
         
}


#define scanlines_strength (4.0*profile.x)
#define beam_min_width     profile.y
#define beam_max_width     profile.z
#define color_boost        profile.w

vec4 get_beam_profile()
{
	vec4 bp = vec4(param.SCANLINES_STRENGTH, param.BEAM_MIN_WIDTH, param.BEAM_MAX_WIDTH, param.COLOR_BOOST);

	if (param.BEAM_PROFILE == 1)  bp = vec4(0.62, 1.00, 1.00, 1.40); // Catmull-rom
	if (param.BEAM_PROFILE == 2)  bp = vec4(0.72, 1.00, 1.00, 1.20); // Catmull-rom

	return bp;
}


void main()
{
    vec4 profile = get_beam_profile();

    vec2 TextureSize = mix(vec2(global.SourceSize.x * param.SHARPNESS_HACK, global.SourceSize.y), vec2(global.SourceSize.x, global.SourceSize.y * param.SHARPNESS_HACK), param.VSCANLINES);

    vec2 dx = mix(vec2(1.0/TextureSize.x, 0.0), vec2(0.0, 1.0/TextureSize.y), param.VSCANLINES);
    vec2 dy = mix(vec2(0.0, 1.0/TextureSize.y), vec2(1.0/TextureSize.x, 0.0), param.VSCANLINES);

    vec2 pix_coord = vTexCoord.xy*TextureSize + vec2(-0.5, 0.5);

    vec2 tc = mix((floor(pix_coord) + vec2(0.5, 0.5))/TextureSize, (floor(pix_coord) + vec2(1.5, -0.5))/TextureSize, param.VSCANLINES);

    vec2 fp = mix(fract(pix_coord), fract(pix_coord.yx), param.VSCANLINES);

    vec3 c00 = GAMMA_IN(texture(Source, tc     - dx - dy).xyz);
    vec3 c01 = GAMMA_IN(texture(Source, tc          - dy).xyz);
    vec3 c02 = GAMMA_IN(texture(Source, tc     + dx - dy).xyz);
    vec3 c03 = GAMMA_IN(texture(Source, tc + 2.0*dx - dy).xyz);
    vec3 c10 = GAMMA_IN(texture(Source, tc     - dx     ).xyz);
    vec3 c11 = GAMMA_IN(texture(Source, tc              ).xyz);
    vec3 c12 = GAMMA_IN(texture(Source, tc     + dx     ).xyz);
    vec3 c13 = GAMMA_IN(texture(Source, tc + 2.0*dx     ).xyz);

    mat4x4 invX = get_hfilter_profile();

    mat4x3 color_matrix0 = mat4x3(c00, c01, c02, c03);
    mat4x3 color_matrix1 = mat4x3(c10, c11, c12, c13);
    
    vec4 invX_Px    = vec4(fp.x*fp.x*fp.x, fp.x*fp.x, fp.x, 1.0) * invX;
    vec3 color0     = color_matrix0 * invX_Px;
    vec3 color1     = color_matrix1 * invX_Px;

    // Get min/max samples
    vec3 min_sample0 = min(c01,c02);
    vec3 max_sample0 = max(c01,c02);
    vec3 min_sample1 = min(c11,c12);
    vec3 max_sample1 = max(c11,c12);
    
    // Anti-ringing
    vec3 aux = color0;
    color0 = clamp(color0, min_sample0, max_sample0);
    color0 = mix(aux, color0, param.CRT_ANTI_RINGING * step(0.0, (c00-c01)*(c02-c03)));
    aux = color1;
    color1 = clamp(color1, min_sample1, max_sample1);
    color1 = mix(aux, color1, param.CRT_ANTI_RINGING * step(0.0, (c10-c11)*(c12-c13)));

    float pos0 = fp.y;
    float pos1 = 1 - fp.y;

    vec3 lum0 = mix(vec3(beam_min_width), vec3(beam_max_width), color0);
    vec3 lum1 = mix(vec3(beam_min_width), vec3(beam_max_width), color1);

    vec3 d0 = scanlines_strength*pos0/(lum0*lum0+0.0000001);
    vec3 d1 = scanlines_strength*pos1/(lum1*lum1+0.0000001);

    d0 = exp(-d0*d0);
    d1 = exp(-d1*d1);

    vec3 color = color_boost*(color0*d0+color1*d1);            

    vec2 mask_coords =vTexCoord.xy * global.OutputSize.xy;

    mask_coords = mix(mask_coords.xy, mask_coords.yx, param.VSCANLINES);

    color.rgb*=mask_weights(mask_coords,  param.MASK_INTENSITY, int(param.PHOSPHOR_LAYOUT));

    color  = GAMMA_OUT(color);

    FragColor = vec4(color, 1.0);
}

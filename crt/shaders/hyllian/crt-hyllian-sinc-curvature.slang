#version 450

layout(push_constant) uniform Push
{
    float BEAM_MIN_WIDTH;
    float BEAM_MAX_WIDTH;
    float SCANLINES_STRENGTH;
    float COLOR_BOOST;
    float SHARPNESS_HACK;
    float PHOSPHOR_LAYOUT;
    float MASK_INTENSITY;
    float InputGamma;
    float OutputGamma;
    float VSCANLINES;
    float CRT_ANTI_RINGING;
   float CRT_CURVATURE;
   float CRT_warpX;
   float CRT_warpY;
   float CRT_cornersize;
   float CRT_cornersmooth;
} param;

#pragma parameter CRT_HYLLIAN_SINC "[CRT-HYLLIAN-SINC PARAMS]" 0.0 0.0 0.0 0.0
#pragma parameter BEAM_MIN_WIDTH "    MIN BEAM WIDTH" 0.92 0.0 1.0 0.01
#pragma parameter BEAM_MAX_WIDTH "    MAX BEAM WIDTH" 1.0 0.0 1.0 0.01
#pragma parameter SCANLINES_STRENGTH "    SCANLINES STRENGTH" 0.72 0.0 1.0 0.01
#pragma parameter COLOR_BOOST "    COLOR BOOST" 1.60 1.0 2.0 0.05
#pragma parameter SHARPNESS_HACK "    SHARPNESS_HACK" 1.0 1.0 4.0 1.0
#pragma parameter PHOSPHOR_LAYOUT "    PHOSPHOR LAYOUT" 4.0 0.0 24.0 1.0
#pragma parameter MASK_INTENSITY "    MASK INTENSITY" 0.7 0.0 1.0 0.1
#pragma parameter InputGamma "    INPUT GAMMA" 2.4 0.0 5.0 0.1
#pragma parameter OutputGamma "    OUTPUT GAMMA" 2.2 0.0 5.0 0.1
#pragma parameter VSCANLINES "    SCANLINES DIRECTION" 0.0 0.0 1.0 1.0
#pragma parameter CRT_CURVATURE "CRT-Curvature" 1.0 0.0 1.0 1.0
#pragma parameter CRT_warpX "CRT-Curvature X-Axis" 0.015 0.0 0.125 0.005
#pragma parameter CRT_warpY "CRT-Curvature Y-Axis" 0.015 0.0 0.125 0.005
vec2 CRT_Distortion = vec2(param.CRT_warpX, param.CRT_warpY) * 15.;
#pragma parameter CRT_cornersize "CRT-Corner Size" 0.01 0.001 1.0 0.005
#define cornersize param.CRT_cornersize
#pragma parameter CRT_cornersmooth "CRT-Corner Smoothness" 380.0 80.0 2080.0 100.0
#define cornersmooth param.CRT_cornersmooth


#pragma parameter CRT_ANTI_RINGING "CRT Anti-Ringing [ OFF | ON ]" 1.0 0.0 1.0 1.0

#define CRT_ANTI_RINGING param.CRT_ANTI_RINGING

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
    vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 FragCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

/*
    Hyllian's CRT Shader - Sinc/Spline16 version

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

#define scanlines_strength (2.0*param.SCANLINES_STRENGTH)
#define beam_min_width     param.BEAM_MIN_WIDTH
#define beam_max_width     param.BEAM_MAX_WIDTH
#define color_boost        param.COLOR_BOOST

#define pi    3.1415926535897932384626433832795
#define wa    (0.5*pi)
#define wb    (pi)

const vec2 corner_aspect   = vec2(1.0,  0.75);

float corner(vec2 coord)
{
    coord = (coord - vec2(0.5)) + vec2(0.5, 0.5);
    coord = min(coord, vec2(1.0) - coord) * corner_aspect;
    vec2 cdist = vec2(cornersize);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));
    
    return clamp((cdist.x - dist)*cornersmooth, 0.0, 1.0);
}


vec2 Warp(vec2 texCoord){

  vec2 curvedCoords = texCoord * 2.0 - 1.0;
  float curvedCoordsDistance = sqrt(curvedCoords.x*curvedCoords.x+curvedCoords.y*curvedCoords.y);

  curvedCoords = curvedCoords / curvedCoordsDistance;

  curvedCoords = curvedCoords * (1.0-pow(vec2(1.0-(curvedCoordsDistance/1.4142135623730950488016887242097)),(1.0/(1.0+CRT_Distortion*0.2))));

  curvedCoords = curvedCoords / (1.0-pow(vec2(0.29289321881345247559915563789515),(1.0/(vec2(1.0)+CRT_Distortion*0.2))));

  curvedCoords = curvedCoords * 0.5 + 0.5;
  return curvedCoords;
}

float weight(float x)
{
	x = abs(x);

	if (x < 1.0)
	{
		return
			(
			 ((x - 9.0 / 5.0) * x - 1.0 / 5.0 ) * x + 1.0
			);
	}
	else if ((x >= 1.0) && (x < 2.0))
	{
		return
			(
			 (( -1.0 / 3.0 * (x - 1) + 4.0 / 5.0 ) * (x - 1) - 7.0 / 15.0 ) * (x - 1)
			);
	}
	else
	{
		return 0.0;
	}
}

vec4 weight4(float x)
{
	return vec4(
			weight(x - 2.0),
			weight(x - 1.0),
			weight(x),
			weight(x + 1.0)
		   );
}


vec3 resampler3(vec3 x)
{
	vec3 res;
	res.x = (x.x<=0.001) ?  wa*wb  :  sin(x.x*wa)*sin(x.x*wb)/(x.x*x.x);
	res.y = (x.y<=0.001) ?  wa*wb  :  sin(x.y*wa)*sin(x.y*wb)/(x.y*x.y);
	res.z = (x.z<=0.001) ?  wa*wb  :  sin(x.z*wa)*sin(x.z*wb)/(x.z*x.z);
	return res;
}

void main()
{
    vec2 TextureSize = mix(vec2(global.SourceSize.x * param.SHARPNESS_HACK, global.SourceSize.y), vec2(global.SourceSize.x, global.SourceSize.y * param.SHARPNESS_HACK), param.VSCANLINES);

    vec2 dx = mix(vec2(1.0/TextureSize.x, 0.0), vec2(0.0, 1.0/TextureSize.y), param.VSCANLINES);
    vec2 dy = mix(vec2(0.0, 1.0/TextureSize.y), vec2(1.0/TextureSize.x, 0.0), param.VSCANLINES);

    vec2 pp = vTexCoord.xy;
    pp = (param.CRT_CURVATURE > 0.5) ? Warp(pp) : pp;
 
    vec2 pix_coord = pp.xy*TextureSize + vec2(-0.5, 0.5);

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

    mat4x3 color_matrix0 = mat4x3(c00, c01, c02, c03);
    mat4x3 color_matrix1 = mat4x3(c10, c11, c12, c13);

    // Get weights for spline16 horizontal filter
    vec4 weights = weight4(1.0 - fp.x);
    
    // Spline16 horizontal filter
    vec3 color0   = (color_matrix0 * weights)/dot(weights, vec4(1.0));
    vec3 color1   = (color_matrix1 * weights)/dot(weights, vec4(1.0));

    // Get min/max samples
    vec3 min_sample0 = min(c01,c02);
    vec3 max_sample0 = max(c01,c02);
    vec3 min_sample1 = min(c11,c12);
    vec3 max_sample1 = max(c11,c12);
    
    // Anti-ringing
    vec3 aux = color0;
    color0 = clamp(color0, min_sample0, max_sample0);
    color0 = mix(aux, color0, CRT_ANTI_RINGING * step(0.0, (c00-c01)*(c02-c03)));
    aux = color1;
    color1 = clamp(color1, min_sample1, max_sample1);
    color1 = mix(aux, color1, CRT_ANTI_RINGING * step(0.0, (c10-c11)*(c12-c13)));

    // Apply scanlines. Sinc filter vertically.
    float pos0 = fp.y;
    float pos1 = 1 - fp.y;

    vec3 lum0 = mix(vec3(beam_min_width), vec3(beam_max_width), color0);
    vec3 lum1 = mix(vec3(beam_min_width), vec3(beam_max_width), color1);

    vec3 d0 = clamp(scanlines_strength*pos0/(lum0*lum0+0.0000001), 0.0, 1.0);
    vec3 d1 = clamp(scanlines_strength*pos1/(lum1*lum1+0.0000001), 0.0, 1.0);

    d0 = resampler3(d0);
    d1 = resampler3(d1);

    // Apply color enhancement, scanlines orientation, mask and gamma.
    vec3 color = color_boost*(color0*d0+color1*d1)/(wa*wb);

    vec2 mask_coords = vTexCoord.xy * global.OutputSize.xy;

    mask_coords = mix(mask_coords.xy, mask_coords.yx, param.VSCANLINES);

    color.rgb*=mask_weights(mask_coords,  param.MASK_INTENSITY, int(param.PHOSPHOR_LAYOUT));

    color  = GAMMA_OUT(color);

    FragColor = vec4(color, 1.0);

    FragColor *= (param.CRT_CURVATURE > 0.5) ? corner(pp) : 1.0;
}

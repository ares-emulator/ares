#version 450

/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////

//  crt-maximus-royale: A fully customizable extension for crt-royale shader,
//  inside a TV / MONITOR BOX with backlight and some other cool stuff.
//  Copyright (C) 2022 DigiDwrf
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program; if not, write to the Free Software Foundation, Inc., 59 Temple
//  Place, Suite 330, Boston, MA 02111-1307 USA


layout(push_constant) uniform Push
{
   float CVBS_SVIDEO_RGB;
   float h2x;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OriginalSize;
   vec4 SourceSize;
} global;

#pragma parameter h2x "Horizontal Scale" 2.0 0.0 2.0 1.0
#pragma parameter CVBS_SVIDEO_RGB "CVBS - SVIDEO - RGB" 0.0 0.0 2.0 1.0

#include "ntsc_rgbyuv.inc"

#define TAPS 24
const float luma_filter[TAPS + 1] = float[TAPS + 1](
   -0.000012020,
   -0.000022146,
   -0.000013155,
   -0.000012020,
   -0.000049979,
   -0.000113940,
   -0.000122150,
   -0.000005612,
   0.000170516,
   0.000237199,
   0.000169640,
   0.000285688,
   0.000984574,
   0.002018683,
   0.002002275,
   -0.000909882,
   -0.007049081,
   -0.013222860,
   -0.012606931,
   0.002460860,
   0.035868225,
   0.084016453,
   0.135563500,
   0.175261268,
   0.190176552);

const float chroma_filter[TAPS + 1] = float[TAPS + 1](
   -0.000118847,
   -0.000271306,
   -0.000502642,
   -0.000930833,
   -0.001451013,
   -0.002064744,
   -0.002700432,
   -0.003241276,
   -0.003524948,
   -0.003350284,
   -0.002491729,
   -0.000721149,
   0.002164659,
   0.006313635,
   0.011789103,
   0.018545660,
   0.026414396,
   0.035100710,
   0.044196567,
   0.053207202,
   0.061590275,
   0.068803602,
   0.074356193,
   0.077856564,
   0.079052396);

#define fetch_offset(offset, one_x) \
   texture(Source, vTexCoord + vec2((offset) * (one_x), 0.0)).xyz

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float cutH;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   cutH = global.SourceSize.x / 2.0;

   if (params.CVBS_SVIDEO_RGB < 2)
   {
      if (params.h2x == 0.0) { cutH /= 4.0; }
      else if (params.h2x == 1.0) { cutH /= 2.0; }
      else if (global.OriginalSize.x > 500) { cutH /= 2.0; }

      vTexCoord -= vec2(0.5 / cutH, 0.0); // Compensate for decimate-by-2.
   }
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float cutH;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
vec3 rgb;

if (params.CVBS_SVIDEO_RGB < 2)
{
   float one_x = 1.0 / cutH;
   vec3 signal = vec3(0.0);

   for (int i = 0; i < TAPS; i++)
   {
      float offset = float(i);
      vec3 sums = fetch_offset(offset - float(TAPS), one_x) + fetch_offset(float(TAPS) - offset, one_x);
      signal += sums * vec3(luma_filter[i], chroma_filter[i], chroma_filter[i]);
   }

   signal += texture(Source, vTexCoord).xyz * vec3(luma_filter[TAPS], chroma_filter[TAPS], chroma_filter[TAPS]);

   rgb = yiq2rgb(signal);
}
else
{
   rgb = texture(Source, vTexCoord).xyz;
}

FragColor = vec4(rgb, 1.0);
}

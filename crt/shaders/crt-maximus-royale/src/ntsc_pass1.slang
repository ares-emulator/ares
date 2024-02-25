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
   float h2x;
   float CVBS_SVIDEO_RGB;
   float CVBS_RES;
   float CVBS_PHASE;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
} global;

#pragma parameter h2x "Horizontal Scale" 2.0 0.0 2.0 1.0
#pragma parameter CVBS_SVIDEO_RGB "CVBS - SVIDEO - RGB" 0.0 0.0 2.0 1.0
#pragma parameter CVBS_RES "CVBS Intensity" 0.0 0.0 1.0 1.0
#pragma parameter CVBS_PHASE "CVBS Phase" 3.0 2.0 4.0 1.0

#define PI 3.14159265
#define CMF_TWO_PHASE (4.0 * PI / 15.0)
#define CMF_THREE_PHASE (PI / 3.0)

#define FRGART (params.h2x + params.CVBS_RES)

mat3 mix_matCVBS = mat3(
	1.0, FRGART, FRGART,
	FRGART, 2.0, 0.0,
	FRGART, 0.0, 2.0
);

mat3 mix_matSVIDEO = mat3(
	1.0, 0.0, 0.0,
	0.0, 2.0, 0.0,
	0.0, 0.0, 2.0
);

#include "ntsc_rgbyuv.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 pix_no;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;

   if (params.CVBS_SVIDEO_RGB < 2)
   {
      vec2 cutH = vec2(1.0);
      vec2 doubleY = vec2(1.0);
      vec2 Src_Size = global.SourceSize.xy * vec2(0.25, 1.0);
      vec2 Out_Size = global.OutputSize.xy * vec2(0.5, 1.0);

      if (params.h2x == 0.0) { cutH.x = 4.0; }
      if (params.h2x == 1.0) { cutH.x = 2.0; }
      if (global.OriginalSize.y > 400) { doubleY.y = 0.5; }

      vec2 SrcSize = Src_Size / cutH;
      vec2 OutSize = Out_Size / cutH;
      pix_no = TexCoord * SrcSize * (OutSize / SrcSize) * doubleY;
   }
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 pix_no;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
vec3 col = texture(Source, vTexCoord).rgb;

vec3 yiq;
vec3 yiqsum;
float chroma_phase;
float mod_phase;
float i_mod;
float q_mod;
float CHROMA_MOD_FREQ;
float PHASE_MOD_FREQ;

if (params.CVBS_SVIDEO_RGB < 2)
{
   yiq = rgb2yiq(col);

   if (params.CVBS_PHASE == 2) CHROMA_MOD_FREQ = CMF_TWO_PHASE;
   if (params.CVBS_PHASE >= 3) CHROMA_MOD_FREQ = CMF_THREE_PHASE;
   if (params.CVBS_PHASE == 4) PHASE_MOD_FREQ = 2.0;
   else PHASE_MOD_FREQ = 3.0;

   chroma_phase = 0.6667 * PI * (mod(pix_no.y, PHASE_MOD_FREQ) + global.FrameCount);
   mod_phase = chroma_phase + (pix_no.x) * CHROMA_MOD_FREQ;

   i_mod = cos(mod_phase);
   q_mod = sin(mod_phase);

   yiq.yz *= vec2(i_mod, q_mod); // Modulate.

   if (params.CVBS_SVIDEO_RGB == 0) { yiq *= mix_matCVBS; } // Cross-talk. 
   if (params.CVBS_SVIDEO_RGB == 1) { yiq *= mix_matSVIDEO; } // Cross-talk. 
   
   yiq.yz *= vec2(i_mod, q_mod); // Demodulate.

   col = yiq;
}

FragColor = vec4(col, 1.0);
}

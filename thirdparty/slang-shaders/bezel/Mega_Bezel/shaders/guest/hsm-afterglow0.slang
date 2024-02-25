#version 450

/*
   Phosphor Afterglow Shader pass 0
   
   Copyright (C) 2020 - 2023 guest(r) - guest.r@gmail.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
   
*/

layout(push_constant) uniform Push
{
   vec4 DerezedPassSize;
   float PR, PG, PB;
} params;

#pragma parameter GDV_XX_LINE					" " 0 0 0.001 0.001
#pragma parameter GDV_GDV_ADV_TITLE 		"[ --- GUEST ADVANCED --- ]:" 0.0 0.0 1.0 1.0

#pragma parameter GDV_AFTERGLOW_LINE		" " 0 0 0.001 0.001
#pragma parameter bogus_afterglow         "[ AFTERGLOW ]:" 0.0 0.0 1.0 1.0
#pragma parameter PR                      "          Persistence Red -- PR" 0.32 0.0 0.50 0.01
#pragma parameter PG                      "          Persistence Green -- PG"  0.32 0.0 0.50 0.01
#pragma parameter PB                      "          Persistence Blue -- PB"  0.32 0.0 0.50 0.01

#define PR params.PR
#define PG params.PG
#define PB params.PB

#define COMPAT_TEXTURE(c,d) texture(c,d)

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

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
layout(set = 0, binding = 2) uniform sampler2D PreCRTPass;
layout(set = 0, binding = 3) uniform sampler2D AfterglowPassFeedback;

#define TEX0 vTexCoord


void main()
{
	vec2 dx = vec2(params.DerezedPassSize.z, 0.0);
	vec2 dy = vec2(0.0, params.DerezedPassSize.w);
	
	vec3 color0  = COMPAT_TEXTURE(PreCRTPass, TEX0.xy).rgb;
	vec3 color1  = COMPAT_TEXTURE(PreCRTPass, TEX0.xy - dx).rgb;
	vec3 color2  = COMPAT_TEXTURE(PreCRTPass, TEX0.xy + dx).rgb;
	vec3 color3  = COMPAT_TEXTURE(PreCRTPass, TEX0.xy - dy).rgb;
	vec3 color4  = COMPAT_TEXTURE(PreCRTPass, TEX0.xy + dy).rgb;	

	vec3 color = (2.5 * color0 + color1 + color2 + color3 + color4)/6.5;
	
	vec3 accumulate = COMPAT_TEXTURE(AfterglowPassFeedback, TEX0.xy).rgb;

	float w = 1.0;
	if ((color0.r + color0.g + color0.b < 5.0/255.0)) { w = 0.0; }

	vec3 result = mix( max(mix(color, accumulate, 0.49 + vec3(PR, PG, PB))- 1.25/255.0, 0.0), color, w);

	FragColor = vec4(result, w);
}
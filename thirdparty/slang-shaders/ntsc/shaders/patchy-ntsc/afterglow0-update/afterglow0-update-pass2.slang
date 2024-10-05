#version 450

/*
   Phosphor Afterglow Shader pass 0
   
   Copyright (C) 2020 - 2022 guest(r) - guest.r@gmail.com

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
   vec4 SourceSize;
   vec4 OriginalSize;
   vec4 OutputSize;
   uint FrameCount;
   float PR_upd, PG_upd, PB_upd;
} params;

#pragma parameter PR_upd "Afterglow Persistence Red" 0.32 0.0 0.50 0.01
#pragma parameter PG_upd "Afterglow Persistence Green"  0.32 0.0 0.50 0.01
#pragma parameter PB_upd "Afterglow Persistence Blue"  0.32 0.0 0.50 0.01

#define PR params.PR_upd
#define PG params.PG_upd
#define PB params.PB_upd

#define COMPAT_TEXTURE(c,d) texture(c,d)

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma name AfterglowUpd

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
layout(set = 0, binding = 3) uniform sampler2D AfterglowUpdFeedback;

#define TEX0 vTexCoord


void main()
{
	vec2 dx = vec2(params.OriginalSize.z, 0.0);
	vec2 dy = vec2(0.0, params.OriginalSize.w);
	
	vec3 color0  = COMPAT_TEXTURE(Source, TEX0.xy).rgb;
	vec3 color1  = COMPAT_TEXTURE(Source, TEX0.xy - dx).rgb;
	vec3 color2  = COMPAT_TEXTURE(Source, TEX0.xy + dx).rgb;
	vec3 color3  = COMPAT_TEXTURE(Source, TEX0.xy - dy).rgb;
	vec3 color4  = COMPAT_TEXTURE(Source, TEX0.xy + dy).rgb;	

//	vec3 color = (2.5 * color0 + color1 + color2 + color3 + color4)/6.5;
    vec3 color = color0;
	
	vec3 accumulate = COMPAT_TEXTURE(AfterglowUpdFeedback, TEX0.xy).rgb;

	float w = 1.0;
	if ((color0.r + color0.g + color0.b < 5.0/255.0)) { w = 0.0; }

	vec3 result = mix( max(mix(color, accumulate, 0.49 + vec3(PR, PG, PB))- 1.25/255.0, 0.0), color, w);

	FragColor = vec4(result, w);
}

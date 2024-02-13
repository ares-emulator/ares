#version 450

/*
   Copyright (C) 2007 guest(r) - guest.r@gmail.com

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
	float AAOFFSET2;
} params;

#pragma parameter AAOFFSET2 "AA offset second pass" 0.5 0.25 2.0 0.05 

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
   vTexCoord = TexCoord * 1.00001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   vec2 texsize = params.SourceSize.xy;
   float dx = pow(texsize.x, -1.0) * params.AAOFFSET2;
   float dy = pow(texsize.y, -1.0) * params.AAOFFSET2;
   vec3 dt = vec3(1.0, 1.0, 1.0);

   vec2 UL = vTexCoord.xy + vec2(-dx,-dy);
   vec2 UR = vTexCoord.xy + vec2(dx,-dy);
   vec2 DL = vTexCoord.xy + vec2(-dx, dy);
   vec2 DR = vTexCoord.xy + vec2(dx, dy);

   vec3 c00 = texture(Source, UL).xyz;
   vec3 c20 = texture(Source, UR).xyz;
   vec3 c02 = texture(Source, DL).xyz;
   vec3 c22 = texture(Source, DR).xyz;

   float m1=dot(abs(c00-c22),dt)+0.001;
   float m2=dot(abs(c02-c20),dt)+0.001;

   FragColor = vec4((m1*(c02+c20)+m2*(c22+c00))/(2.0*(m1+m2)),1.0);
}
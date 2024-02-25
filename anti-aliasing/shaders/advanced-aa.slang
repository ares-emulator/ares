#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float AA_RESOLUTION_X;
	float AA_RESOLUTION_Y;
} params;

#pragma parameter AA_RESOLUTION_X "AA Input Res X" 0.0 0.0 1920.0 1.0
#pragma parameter AA_RESOLUTION_Y "AA Input Res Y" 0.0 0.0 1920.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

/*
   Copyright (C) 2006 guest(r) - guest.r@gmail.com

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

/*
   The AdvancedAA shader is well used to:
   - AA 2xscaled gfx. to its 4x absolute size,   
   - AA hi-res "screens" (640x480) to their 2x size or,
   - AA gfx. back to it's original size (looks nice above 640x480, set scaling to 1.0) 
*/

#define AA_RESOLUTION_X_DEF params.SourceSize.x
#define AA_RESOLUTION_Y_DEF params.SourceSize.y
vec3  dt = vec3(1,1,1);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out vec4 t3;
layout(location = 4) out vec4 t4;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
   	vec2 ps = vec2(1.0/((params.AA_RESOLUTION_X == 0) ? AA_RESOLUTION_X_DEF : params.AA_RESOLUTION_X), 1.0/((params.AA_RESOLUTION_Y == 0) ? AA_RESOLUTION_Y_DEF : params.AA_RESOLUTION_Y));
	float dx = ps.x*0.5;
	float dy = ps.y*0.5;
	
	t1.xy = vTexCoord + vec2(-dx, 0);
	t2.xy = vTexCoord + vec2( dx, 0);
	t3.xy = vTexCoord + vec2( 0,-dy);
	t4.xy = vTexCoord + vec2( 0, dy);
	t1.zw = vTexCoord + vec2(-dx,-dy);
	t2.zw = vTexCoord + vec2(-dx, dy);
	t3.zw = vTexCoord + vec2( dx,-dy);
	t4.zw = vTexCoord + vec2( dx, dy);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 4) in vec4 t4;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   vec3 c00 = texture(Source, t1.zw).xyz; 
   vec3 c10 = texture(Source, t3.xy).xyz;
   vec3 c20 = texture(Source, t3.zw).xyz;
   vec3 c01 = texture(Source, t1.xy).xyz;
   vec3 c11 = texture(Source, vTexCoord).xyz;
   vec3 c21 = texture(Source, t2.xy).xyz;
   vec3 c02 = texture(Source, t2.zw).xyz;
   vec3 c12 = texture(Source, t4.xy).xyz;
   vec3 c22 = texture(Source, t4.zw).xyz;

   float d1=dot(abs(c00-c22),dt)+0.0001;
   float d2=dot(abs(c20-c02),dt)+0.0001;
   float hl=dot(abs(c01-c21),dt)+0.0001;
   float vl=dot(abs(c10-c12),dt)+0.0001;
   
   float k1=0.5*(hl+vl);
   float k2=0.5*(d1+d2);

   vec3 t1=(hl*(c10+c12)+vl*(c01+c21)+k1*c11)/(2.5*(hl+vl));
   vec3 t2=(d1*(c20+c02)+d2*(c00+c22)+k2*c11)/(2.5*(d1+d2));

   k1=dot(abs(t1-c11),dt)+0.0001;
   k2=dot(abs(t2-c11),dt)+0.0001;
   
   FragColor = vec4((k1*t2+k2*t1)/(k1+k2),1);
}
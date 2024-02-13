#version 450

/*
   Sharpsmoother shader
   
   Copyright (C) 2005-2017 guest(r) - guest.r@gmail.com

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
	float max_w;
	float min_w;
	float smoot;
	float lumad;
	float mtric;
	float SHARPSMOOTHER_ON;
} params;

// Original Values:
// max_w = "0.1"
// min_w = "-0.07"
// smoot = "0.55"
// lumad = "0.3"
// mtric = "0.7"

// New default values tuned by Hyllian
#pragma parameter HSM_SHARPSMOOTHER_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter SHARPSMOOTHER_TITLE     "[ --- SHARPSMOOTHER by Guest.r --- ] "  0  0 0.0.001 0.001
#pragma parameter SHARPSMOOTHER_ON        "          SharpSmoother ON"     0     0 1 1
#define SHARPSMOOTHER_ON params.SHARPSMOOTHER_ON

#pragma parameter max_w                   "          Max filter weight"    0.05  0.00 0.20 0.01
#pragma parameter min_w                   "          Min filter weight"    0.00 -0.15 0.05 0.01
#pragma parameter smoot                   "          Smoothing strength"   0.4   0.00 1.50 0.01
#pragma parameter lumad                   "          Effects smoothing"    0.5   0.10 5.00 0.10
#pragma parameter mtric                   "          Metric Used"          0.3   0.10 2.00 0.10

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

vec3 dt = vec3(1.0, 1.0, 1.0);


float wt(vec3 A, vec3 B)
{	
	return clamp(params.smoot - ((6.0+params.lumad)/pow(3.0,params.mtric))*pow(dot(pow(abs(A-B),vec3(1.0/params.mtric)),dt),params.mtric)/(dot(A+B,dt)+params.lumad), params.min_w, params.max_w);
}

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

   if (SHARPSMOOTHER_ON < 0.5)
   {
      return;
   }
   
   float x = 1.0 * (1.0 / params.SourceSize.x);
   float y = 1.0 * (1.0 / params.SourceSize.y);
   vec2 dg1 = vec2( x, y);
   vec2 dg2 = vec2(-x, y);
   vec2 dx = vec2(x, 0.0);
   vec2 dy = vec2(0.0, y);
   t1 = vec4(vTexCoord.xy-dg1,vTexCoord.xy-dy);
   t2 = vec4(vTexCoord.xy-dg2,vTexCoord.xy+dx);
   t3 = vec4(vTexCoord.xy+dg1,vTexCoord.xy+dy);
   t4 = vec4(vTexCoord.xy+dg2,vTexCoord.xy-dx);
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
   if (SHARPSMOOTHER_ON < 0.5)
   {
      FragColor = texture(Source, vTexCoord);
      return;
   }
   
   vec3 c00 = texture(Source, t1.xy).xyz; 
   vec3 c10 = texture(Source, t1.zw).xyz; 
   vec3 c20 = texture(Source, t2.xy).xyz; 
   vec3 c01 = texture(Source, t4.zw).xyz; 
   vec3 c11 = texture(Source, vTexCoord.xy).xyz; 
   vec3 c21 = texture(Source, t2.zw).xyz; 
   vec3 c02 = texture(Source, t4.xy).xyz; 
   vec3 c12 = texture(Source, t3.zw).xyz; 
   vec3 c22 = texture(Source, t3.xy).xyz;
       
   float w10 = wt(c11,c10);
   float w21 = wt(c11,c21);
   float w12 = wt(c11,c12);
   float w01 = wt(c11,c01);
   float w00 = wt(c11,c00)*0.75;
   float w22 = wt(c11,c22)*0.75;
   float w20 = wt(c11,c20)*0.75;
   float w02 = wt(c11,c02)*0.75;

   FragColor = vec4(w10*c10+w21*c21+w12*c12+w01*c01+w00*c00+w22*c22+w20*c20+w02*c02+(1.0-w10-w21-w12-w01-w00-w22-w20-w02)*c11, 1.0);
}
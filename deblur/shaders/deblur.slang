#version 450

/*
   Deblur Shader
   
   Copyright (C) 2005 - 2019 guest(r) - guest.r@gmail.com

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
	float OFFSET, DEBLUR, SMART;
} params;

#pragma parameter OFFSET "Deblur offset" 2.0 0.5 4.0 0.25 
#pragma parameter DEBLUR "Deblur str.  " 4.5 1.0 7.0 0.25 
#pragma parameter SMART  "Smart deblur " 0.5 0.0 1.0 0.05 

#define OFFSET params.OFFSET
#define DEBLUR params.DEBLUR
#define SMART params.SMART

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
   vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

vec3  dt = vec3(1.,1.,1.);
vec3  dtt = vec3(0.0001, 0.0001, 0.0001); 

float wt(vec3 A, vec3 B)
{	
	return 4.0*length(A-B)/(dot(A+B,dt)+0.33);
} 

void main()
{
   vec2 tex = vTexCoord;	
   vec2 texsize = params.SourceSize.xy;
   float dx = OFFSET/texsize.x;
   float dy = OFFSET/texsize.y;
   
   vec4 yx = vec4( dx, dy,-dx,-dy);
   vec2 xx = vec2( dx, 0.0);
   vec2 yy = vec2( 0.0, dy);
   
   vec3 c11 = texture(Source, tex        ).xyz;  
   vec3 c00 = texture(Source, tex + yx.zw).xyz;
   vec3 c20 = texture(Source, tex + yx.xw).xyz;
   vec3 c22 = texture(Source, tex + yx.xy).xyz;
   vec3 c02 = texture(Source, tex + yx.zy).xyz;
   vec3 c10 = texture(Source, tex - yy   ).xyz;     
   vec3 c21 = texture(Source, tex + xx   ).xyz;
   vec3 c12 = texture(Source, tex + yy   ).xyz;
   vec3 c01 = texture(Source, tex - xx   ).xyz;
   
   vec3 mn1 = min (min (c00,c01),c02);
   vec3 mn2 = min (min (c10,c11),c12);
   vec3 mn3 = min (min (c20,c21),c22);
   vec3 mx1 = max (max (c00,c01),c02);
   vec3 mx2 = max (max (c10,c11),c12);
   vec3 mx3 = max (max (c20,c21),c22);

   vec3 d11;
   
   mn1 = min(min(mn1,mn2),mn3);
   mx1 = max(max(mx1,mx2),mx3);
   vec3 contrast = mx1 - mn1;
	
   vec3 dif1 = abs(c11-mn1) + dtt;
   vec3 dif2 = abs(c11-mx1) + dtt;
   
   float DB1 = DEBLUR; float dif;
   
   dif1=vec3(pow(dif1.x,DB1),pow(dif1.y,DB1),pow(dif1.z,DB1));
   dif2=vec3(pow(dif2.x,DB1),pow(dif2.y,DB1),pow(dif2.z,DB1));

   d11 = vec3((dif1.x*mx1.x + dif2.x*mn1.x)/(dif1.x + dif2.x),
                (dif1.y*mx1.y + dif2.y*mn1.y)/(dif1.y + dif2.y),
                (dif1.z*mx1.z + dif2.z*mn1.z)/(dif1.z + dif2.z));   

   float k10 = 1.0/(dot(abs(c10-d11),dt)+0.0001);
   float k01 = 1.0/(dot(abs(c01-d11),dt)+0.0001);
   float k11 = 1.0/(dot(abs(c11-d11),dt)+0.0001);  
   float k21 = 1.0/(dot(abs(c21-d11),dt)+0.0001);
   float k12 = 1.0/(dot(abs(c12-d11),dt)+0.0001);   
   float k00 = 1.0/(dot(abs(c00-d11),dt)+0.0001);
   float k02 = 1.0/(dot(abs(c02-d11),dt)+0.0001);  
   float k20 = 1.0/(dot(abs(c20-d11),dt)+0.0001);
   float k22 = 1.0/(dot(abs(c22-d11),dt)+0.0001);   
   
   float avg = (k10+k01+k11+k21+k12+k00+k02+k20+k22)/30.0;
   
   k10 = max(k10-avg, 0.0);
   k01 = max(k01-avg, 0.0);
   k11 = max(k11-avg, 0.0);   
   k21 = max(k21-avg, 0.0);
   k12 = max(k12-avg, 0.0);
   k00 = max(k00-avg, 0.0);
   k02 = max(k02-avg, 0.0);   
   k20 = max(k20-avg, 0.0);
   k22 = max(k22-avg, 0.0);
   
   d11 = (k10*c10 + k01*c01 + k11*c11 + k21*c21 + k12*c12 + k00*c00 + k02*c02 + k20*c20 + k22*c22 + 0.0001*c11)/(k10+k01+k11+k21+k12+k00+k02+k20+k22+0.0001);
   
   c11 = mix(c11, d11, clamp(1.75*contrast-0.125, 0.0, 1.0));
   c11 = mix(d11, c11, SMART);   
   
   FragColor = vec4(c11,1.0); 
}

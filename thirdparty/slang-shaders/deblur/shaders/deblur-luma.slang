#version 450

/*
   Deblur-Luma Shader
   
   Copyright (C) 2005 - 2024 guest(r) - guest.r@gmail.com

   Luma adaptation by Hyllian 	

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

#pragma parameter OFFSET "Deblur offset" 2.0 0.25 4.0 0.25 
#pragma parameter DEBLUR "Deblur str.  " 4.5 1.0 7.0 0.25 
#pragma parameter SMART  "Smart deblur " 0.5 0.0 1.0 0.05 

#define OFFSET params.OFFSET
#define DEBLUR params.DEBLUR
#define SMART params.SMART

const vec3 luma = vec3(0.299,0.587,0.114);
const vec4  res = vec4(0.0001, 0.0001, 0.0001, 0.0001);
const vec4  uno = vec4(1.,1.,1.,1.);

float min8(vec4 a4, vec4 b4)
{
    vec4 ab4 = min(a4, b4); vec2 ab2 = min(ab4.xy, ab4.zw); return min(ab2.x, ab2.y);
}

float max8(vec4 a4, vec4 b4)
{
    vec4 ab4 = max(a4, b4); vec2 ab2 = max(ab4.xy, ab4.zw); return max(ab2.x, ab2.y);
}

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out vec4 t3;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;

   float dx = OFFSET*params.SourceSize.z;
   float dy = OFFSET*params.SourceSize.w;

   t1 = vTexCoord.xxxy + vec4( -dx, 0, dx, -dy); //  c00 c10 c20
   t2 = vTexCoord.xxxy + vec4( -dx, 0, dx,   0); //  c01 c11 c21
   t3 = vTexCoord.xxxy + vec4( -dx, 0, dx,  dy); //  c02 c12 c22
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


void main()
{
   vec3 c11 = texture(Source, t2.yw).xyz;  
   vec3 c00 = texture(Source, t1.xw).xyz;
   vec3 c20 = texture(Source, t1.zw).xyz;
   vec3 c22 = texture(Source, t3.zw).xyz;
   vec3 c02 = texture(Source, t3.xw).xyz;
   vec3 c10 = texture(Source, t1.yw).xyz;     
   vec3 c21 = texture(Source, t2.zw).xyz;
   vec3 c12 = texture(Source, t3.yw).xyz;
   vec3 c01 = texture(Source, t2.xw).xyz;

   mat4x3 chv = mat4x3(c10, c01, c21, c12);
   mat4x3 cdi = mat4x3(c00, c02, c20, c22);

   vec4  CHV = luma * chv;
   vec4  CDI = luma * cdi;
   float C11 = dot(c11, luma);

   float mn1 = min8(CHV, CDI);
   float mx1 = max8(CHV, CDI);
 
   vec2 mnmx = vec2(min(C11, mn1), max(C11, mx1));

   vec2 dif = abs(vec2(C11) - mnmx) + res.xy;
   
   dif = pow(dif, vec2(DEBLUR));

   float D11 = dot(dif, mnmx.yx)/(dif.x + dif.y);   

   float k11 = 1.0/(abs(C11 - D11) + res.x);  

   vec4 khv = vec4(1.0/(abs(CHV-vec4(D11)) + res));
   vec4 kdi = vec4(1.0/(abs(CDI-vec4(D11)) + res));

   float avg = (dot(khv + kdi, uno) + k11)/10.0;
   
   khv = max(khv-vec4(avg), vec4(0.0));
   kdi = max(kdi-vec4(avg), vec4(0.0));
   k11 = max(k11-avg, 0.0);

   vec3 d11 = (chv*khv + cdi*kdi + (k11 + res.x)*c11) / (dot(khv + kdi, uno) + k11 + res.x);

   float contrast = mnmx.y - mnmx.x;
   c11 = mix(c11, d11, clamp(1.75*contrast-0.125, 0.0, 1.0));
   c11 = mix(d11, c11, SMART);   
   
   FragColor = vec4(c11,1.0);
}

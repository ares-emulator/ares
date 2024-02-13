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
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

const float     floatpi     = 1.5707963267948966192313216916398;
const float         pi     = 3.1415926535897932384626433832795;

vec4 l(vec4 x)
{
   vec4 res;

   res.x = (x.x==0.0) ?  (pi*floatpi)  :  sin(x.x*floatpi)*sin(x.x*pi)/(x.x*x.x);
   res.y = (x.y==0.0) ?  (pi*floatpi)  :  sin(x.y*floatpi)*sin(x.y*pi)/(x.y*x.y);
   res.z = (x.z==0.0) ?  (pi*floatpi)  :  sin(x.z*floatpi)*sin(x.z*pi)/(x.z*x.z);
   res.w = (x.w==0.0) ?  (pi*floatpi)  :  sin(x.w*floatpi)*sin(x.w*pi)/(x.w*x.w);

   return res;
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
   vTexCoord = TexCoord * 1.0001;
   vec2 ps = params.SourceSize.zw;
   float dx = ps.x;
   float dy = ps.y;
   
   t1 = vTexCoord.xxyy + vec4(   -dx,    0.0,    -dy,    0.0);
   t2 = vTexCoord.xxyy + vec4(    dx, 2.0*dx,    -dy,    0.0);
   t3 = vTexCoord.xxyy + vec4(   -dx,    0.0,     dy, 2.0*dy);
   t4 = vTexCoord.xxyy + vec4(    dx, 2.0*dx,     dy, 2.0*dy);
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
   mat4x3 pix;
   vec2 fraction = fract(vTexCoord*params.SourceSize.xy);

   // calculating texel weights

   vec4 abcd, pqrs;

   abcd = l(vec4(1+fraction.x, fraction.x, 1-fraction.x, 2-fraction.x));
   pqrs = l(vec4(1+fraction.y, fraction.y, 1-fraction.y, 2-fraction.y));

   // reading the texels

   vec3 c00 = texture(Source, t1.xz).xyz;
   vec3 c10 = texture(Source, t1.yz).xyz;
   vec3 c20 = texture(Source, t2.xz).xyz;
   vec3 c30 = texture(Source, t2.yz).xyz;
   vec3 c01 = texture(Source, t1.xw).xyz;
   vec3 c11 = texture(Source, vTexCoord).xyz;
   vec3 c21 = texture(Source, t2.xw).xyz;
   vec3 c31 = texture(Source, t2.yw).xyz;
   vec3 c02 = texture(Source, t3.xz).xyz;
   vec3 c12 = texture(Source, t3.yz).xyz;
   vec3 c22 = texture(Source, t4.xz).xyz;
   vec3 c32 = texture(Source, t4.yz).xyz;
   vec3 c03 = texture(Source, t3.xw).xyz;
   vec3 c13 = texture(Source, t3.yw).xyz;
   vec3 c23 = texture(Source, t4.xw).xyz;
   vec3 c33 = texture(Source, t4.yw).xyz;

   pix[0] = mat4x3(-c00, c10, c20, -c30) * abcd;
   pix[1] = mat4x3( c01, c11, c21,  c31) * abcd;
   pix[2] = mat4x3( c02, c12, c22,  c32) * abcd;
   pix[3] = mat4x3(-c03, c13, c23, -c33) * abcd;

   // final sum and weight normalization
   FragColor = vec4(((pix * pqrs))/((dot(abcd, vec4(1.0))*dot(pqrs, vec4(1.0)))-2*(abcd.x+abcd.w)*(pqrs.x+pqrs.w)),1);
}

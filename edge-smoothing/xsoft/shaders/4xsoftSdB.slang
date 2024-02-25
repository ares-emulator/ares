#version 450

/* 4xSoft Smart deBlur shader
   
   Copyright (C) 2016 guest(r) - guest.r@gmail.com

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
	float RESOLUTION_X;
	float RESOLUTION_Y;
	float CONTRAST;
} params;

#pragma parameter RESOLUTION_X "4xSoft Input Resolution X" 0.0 0.0 1920.0 1.0
#define RESOLUTION_X params.RESOLUTION_X
#pragma parameter RESOLUTION_Y "4xSoft Input Resolution Y" 0.0 0.0 1920.0 1.0
#define RESOLUTION_Y params.RESOLUTION_Y
#pragma parameter CONTRAST     "4xSoft Contrast"           3.0 0.0 10.0 0.1
#define CONTRAST params.CONTRAST

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

const vec3 dt = vec3(1.0, 1.0, 1.0);
const vec3 dtt = vec3(0.001,0.001,0.001);

#define RESOLUTION_X_DEF params.SourceSize.x
#define RESOLUTION_Y_DEF params.SourceSize.y

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out vec4 t3;
layout(location = 4) out vec4 t4;
layout(location = 5) out vec4 t5;
layout(location = 6) out vec4 t6;

void main()
{
   /* messy I know but we need to make it possible to have it default to input resolution x/y in case RESOLUTION_X is 0.0 */
	vec2 ps = vec2(1.0/((RESOLUTION_X == 0) ? RESOLUTION_X_DEF : RESOLUTION_X), 1.0/((RESOLUTION_Y == 0) ? RESOLUTION_Y_DEF : RESOLUTION_Y));

	float dx = ps.x;
	float dy = ps.y;
	float sx = ps.x * 0.5;
	float sy = ps.y * 0.5;
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   	t1 =	vec4(vTexCoord,vTexCoord) + vec4(-dx, -dy, dx, -dy); // outer diag. texels
	t2 =	vec4(vTexCoord,vTexCoord) + vec4(dx, dy, -dx, dy);
	t3 =	vec4(vTexCoord,vTexCoord) + vec4(-sx, -sy, sx, -sy); // inner diag. texels
	t4 =	vec4(vTexCoord,vTexCoord) + vec4(sx, sy, -sx, sy);
	t5 =	vec4(vTexCoord,vTexCoord) + vec4(-dx, 0, dx, 0); // inner hor/vert texels
	t6 =	vec4(vTexCoord,vTexCoord) + vec4(0, -dy, 0, dy);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 4) in vec4 t4;
layout(location = 5) in vec4 t5;
layout(location = 6) in vec4 t6;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
  vec3 c11 = texture(Source, vTexCoord).xyz;
  vec3 c00 = texture(Source, t1.xy).xyz;
  vec3 c20 = texture(Source, t1.zw).xyz;
  vec3 c22 = texture(Source, t2.xy).xyz;
  vec3 c02 = texture(Source, t2.zw).xyz;
  vec3 s00 = texture(Source, t3.xy).xyz;
  vec3 s20 = texture(Source, t3.zw).xyz;
  vec3 s22 = texture(Source, t4.xy).xyz;
  vec3 s02 = texture(Source, t4.zw).xyz;
  vec3 c01 = texture(Source, t5.xy).xyz;
  vec3 c21 = texture(Source, t5.zw).xyz;
  vec3 c10 = texture(Source, t6.xy).xyz;
  vec3 c12 = texture(Source, t6.zw).xyz;

  float d1=dot(abs(c00-c22),dt)+0.0001;
  float d2=dot(abs(c20-c02),dt)+0.0001;
  float hl=dot(abs(c01-c21),dt)+0.0001;
  float vl=dot(abs(c10-c12),dt)+0.0001;
  float m1=dot(abs(s00-s22),dt)+0.0001;
  float m2=dot(abs(s02-s20),dt)+0.0001;

  vec3 mn1 = min (min (c00,c01),c02);
  vec3 mn2 = min (min (c10,c11),c12);
  vec3 mn3 = min (min (c20,c21),c22);
  vec3 mx1 = max (max (c00,c01),c02);
  vec3 mx2 = max (max (c10,c11),c12);
  vec3 mx3 = max (max (c20,c21),c22);

  mn1 = min(min(mn1,mn2),mn3);
  mx1 = max(max(mx1,mx2),mx3);

  vec3 t1=(hl*(c10+c12)+vl*(c01+c21)+(hl+vl)*c11)/(3.0*(hl+vl));
  vec3 t2=(d1*(c20+c02)+d2*(c00+c22)+(d1+d2)*c11)/(3.0*(d1+d2));

  c11 = 0.25*(t1+t2+(m2*(s00+s22)+m1*(s02+s20))/(m1+m2));

  vec3 dif1 = abs(c11-mn1) + dtt;
  vec3 dif2 = abs(c11-mx1) + dtt;

//	float filterparam = 2.0;

  float dif = max(length(dif1),length(dif2));
  float filterparam = clamp(2.25*dif,1.0,2.0);
  
  dif1=vec3(pow(dif1.x,filterparam),pow(dif1.y,filterparam),pow(dif1.z,filterparam));
  dif2=vec3(pow(dif2.x,filterparam),pow(dif2.y,filterparam),pow(dif2.z,filterparam));

  c11 = vec3((dif1.x*mx1.x + dif2.x*mn1.x)/(dif1.x + dif2.x),
               (dif1.y*mx1.y + dif2.y*mn1.y)/(dif1.y + dif2.y),
			   (dif1.z*mx1.z + dif2.z*mn1.z)/(dif1.z + dif2.z));
   FragColor = vec4(c11,1.0);
}
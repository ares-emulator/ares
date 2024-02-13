#version 450

/*
   Hyllian's Deposterize Shader - Pass0
   
   Copyright (C) 2011/2016 Hyllian/Jararaca - sergiogdb@gmail.com

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
	float EQ_THRESH1;
	float DIFF_THRESH1;
} params;

#pragma parameter EQ_THRESH1 "Eq Limit Horizontal" 0.01 0.0 1.0 0.01
#pragma parameter DIFF_THRESH1 "Diff Limit Horizontal" 0.06 0.0 1.0 0.01

#define EQ   params.EQ_THRESH1
#define DF  params.DIFF_THRESH1

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

vec3 df3(vec3 c1, vec3 c2)
{
      return abs(c1 - c2);
}

bvec3 eq3(vec3 A, vec3 B)
{
	return lessThanEqual(df3(A, B) , vec3(EQ));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   
   	vec2 ps = vec2(params.SourceSize.zw);
	float dx = ps.x;
	float dy = ps.y;
	t1 = vTexCoord.xxxy + vec4( -dx, 0, dx,      0); //  D  E  F
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	vec3 res;

	vec3 D  = texture(Source, t1.xw).rgb;
	vec3 E  = texture(Source, t1.yw).rgb;
	vec3 F  = texture(Source, t1.zw).rgb;
	
	bvec3 test1 = eq3(D, F);
	bvec3 test2 = eq3(D, E);
	bvec3 test3 = eq3(E, F);
	
	bvec3 test4 = lessThanEqual(df3(E, F) , vec3(DF, DF, DF));
	bvec3 test5 = lessThanEqual(df3(D, E) , vec3(DF, DF, DF));

	res = ((test1 == bvec3(false)) && ((test4 == bvec3(true)) && (test2 == bvec3(true)) || (test5 == bvec3(true)) && (test3 == bvec3(true)))) ? 0.5*(D+F) : E;

   FragColor = vec4(res, 1.0);
}
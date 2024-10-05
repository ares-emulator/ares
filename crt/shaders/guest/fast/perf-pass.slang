#version 450

/*
   CRT Advanced Fastest Performance Pass
   
   Copyright (C) 2019-2023 guest(r)
   
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
   float fcompat;
   float fbenchmode;
} params;

#pragma parameter bogus_compat "[ COMPATIBILITY AND PERFORMANCE ]:" 0.0 0.0 1.0 1.0
#pragma parameter fcompat "          Compatibility Mode (slower, use if glitchy)" 0.0 0.0 1.0 1.0 


layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

#define gl_FragCoord (vTexCoord * params.OutputSize.xy)

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.00001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D PerfPassFeedback;


bool same_line (vec2 coord, vec2 dx, vec2 xx)
{
	vec3 ol3 = texture(PerfPassFeedback, coord -xx).rgb;
	vec3 ol2 = texture(PerfPassFeedback, coord -dx).rgb;
	vec3 ol1 = texture(PerfPassFeedback, coord    ).rgb;
	vec3 or1 = texture(PerfPassFeedback, coord +dx).rgb;
	vec3 or2 = texture(PerfPassFeedback, coord +xx).rgb;

	vec3 cl3 = texture(Source, coord -xx).rgb;	
	vec3 cl2 = texture(Source, coord -dx).rgb;
	vec3 cl1 = texture(Source, coord    ).rgb;
	vec3 cr1 = texture(Source, coord +dx).rgb;
	vec3 cr2 = texture(Source, coord +xx).rgb;
	
	vec3 res = abs(ol3-cl3) + abs(ol2-cl2) + abs(ol1-cl1) + abs(or1-cr1) + abs(or2-cr2);
	
	bool result = false;
	if ((res.r+res.g+res.b)< 0.025) result = true;
	return result;
}


void main()
{
	vec3 color = texture(Source, vTexCoord.xy).rgb;

	vec2 dx = vec2(params.OriginalSize.z, 0.0); vec2 xx = dx+dx;
	vec2 dy = vec2(0.0, params.OriginalSize.w);

	float result = 0.0;
	if(same_line(vTexCoord-dy,dx,xx) && same_line(vTexCoord,dx,xx) && same_line(vTexCoord+dy,dx,xx)) result = 1.0;

	if (params.fcompat > 0.5) result = 0.0;

	float refresh1 = 120.0;
	float refresh2 = round(vTexCoord.y*119.0);
	bool frames = (abs(floor(mod(float(params.FrameCount), refresh1)) - refresh2) < 1.5);
	if (frames) result = 0.0;

	FragColor = vec4(color, result);
} 
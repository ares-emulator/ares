#version 450

/*
	Spline36 - PassY - by Hyllian 2022

	Adapted from bicubic source shaders below.

	Spline36 math sources: https://www.panotools.org/dersch/interpolator/interpolator.html
*/



/*
   Copyright (C) 2010 Team XBMC
   http://www.xbmc.org
   Copyright (C) 2011 Stefanos A.
   http://www.opentk.com

This Program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This Program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XBMC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
http://www.gnu.org/copyleft/gpl.html
*/


layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float S36_ANTI_RINGING;
} params;

#pragma parameter S36_ANTI_RINGING "Spline36 Anti-Ringing [ OFF | ON ]" 1.0 0.0 1.0 1.0

#define S36_ANTI_RINGING params.S36_ANTI_RINGING

#define AR_STRENGTH 0.8

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
   vTexCoord = TexCoord  + vec2(0.0001, 0.0001);;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


float weight(float x)
{
	x = abs(x);

	if (x < 1.0)
	{
		return
			(
			 ( (   13.0/11.0  * x - 453.0/ 209.0 ) * x -   3.0/ 209.0  ) * x + 1.0
			);
	}
	else if ((x >= 1.0) && (x < 2.0))
	{
		return
			(
			 ( ( -  6.0/11.0  * (x-1) + 270.0/ 209.0 ) * (x-1) - 156.0/ 209.0  ) *(x-1)
			);
	}
	else if ((x >= 2.0) && (x < 3.0))
	{
		return
			(
			 ( (    1.0/11.0  * (x-2) -  45.0/ 209.0 ) * (x-2) +  26.0/ 209.0  ) *(x-2)
			);
	}
	else
	{
		return 0.0;
	}
}

vec3 weight3(float x)
{
	return vec3(
			weight(x - 1.0),
			weight(x),
			weight(x + 1.0)
		   );
}

	

void main()
{
    vec2 ps = params.SourceSize.zw;
    vec2 pos = vTexCoord.xy + ps * vec2(0.0, 0.5);
    vec2 fp = fract(pos / ps);

    vec2 xystart = (-2.5 - fp) * ps + pos;

    float xpos = xystart.x  + ps.x * 3.0;

    vec3 C0 = texture(Source, vec2(xpos, xystart.y             )).rgb;
    vec3 C1 = texture(Source, vec2(xpos, xystart.y + ps.y      )).rgb;
    vec3 C2 = texture(Source, vec2(xpos, xystart.y + ps.y * 2.0)).rgb;
    vec3 C3 = texture(Source, vec2(xpos, xystart.y + ps.y * 3.0)).rgb;
    vec3 C4 = texture(Source, vec2(xpos, xystart.y + ps.y * 4.0)).rgb;
    vec3 C5 = texture(Source, vec2(xpos, xystart.y + ps.y * 5.0)).rgb; 

    vec3 w1 = weight3(-1.0 - fp.y);
    vec3 w2 = weight3( 2.0 - fp.y);

    float sum   = dot(  w1, vec3(1)) + dot(  w2, vec3(1));
    w1   /= sum;
    w2   /= sum;

    vec3 color = mat3( C0, C1, C2 ) * w1 +  mat3( C3, C4, C5) * w2;

    // Anti-ringing
    if (S36_ANTI_RINGING == 1.0)
    {
        vec3 aux = color;
        vec3 min_sample = min(min(C1, C2), min(C3, C4));
        vec3 max_sample = max(max(C1, C2), max(C3, C4));
        color = clamp(color, min_sample, max_sample);
        color = mix(aux, color, AR_STRENGTH*step(0.0, (C1-C2)*(C3-C4)));

    }

    FragColor = vec4(color, 1.0);
}

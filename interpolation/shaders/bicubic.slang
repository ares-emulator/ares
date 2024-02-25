#version 450

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
   float B, C;
} params;

// Default to Mitchel-Netravali coefficients for best psychovisual result
// bicubic-sharp is B = 0.1 and C = 0.5
// bicubic-sharper is B = 0.0 and C = 0.75
#pragma parameter B "Bicubic Coeff B" 0.33 0.0 1.0 0.01
#pragma parameter C "Bicubic Coeff C" 0.33 0.0 1.0 0.01

#define B params.B
#define C params.C

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
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

float weight(float x)
{
	float ax = abs(x);

	if (ax < 1.0)
	{
		return
			(
			 pow(x, 2.0) * ((12.0 - 9.0 * B - 6.0 * C) * ax + (-18.0 + 12.0 * B + 6.0 * C)) +
			 (6.0 - 2.0 * B)
			) / 6.0;
	}
	else if ((ax >= 1.0) && (ax < 2.0))
	{
		return
			(
			 pow(x, 2.0) * ((-B - 6.0 * C) * ax + (6.0 * B + 30.0 * C)) +
			 (-12.0 * B - 48.0 * C) * ax + (8.0 * B + 24.0 * C)
			) / 6.0;
	}
	else
	{
		return 0.0;
	}
}
	
vec4 weight4(float x)
{
	return vec4(
			weight(x - 2.0),
			weight(x - 1.0),
			weight(x),
			weight(x + 1.0));
}

vec3 pixel(float xpos, float ypos, sampler2D tex)
{
	return texture(tex, vec2(xpos, ypos)).rgb;
}

vec3 line_run(float ypos, vec4 xpos, vec4 linetaps, sampler2D tex)
{
	return
		pixel(xpos.r, ypos, tex) * linetaps.r +
		pixel(xpos.g, ypos, tex) * linetaps.g +
		pixel(xpos.b, ypos, tex) * linetaps.b +
		pixel(xpos.a, ypos, tex) * linetaps.a;
}

void main()
{
        vec2 stepxy = vec2(1.0/params.SourceSize.x, 1.0/params.SourceSize.y);
        vec2 pos = vTexCoord.xy + stepxy * 0.5;
        vec2 f = fract(pos / stepxy);
		
	vec4 linetaps   = weight4(1.0 - f.x);
	vec4 columntaps = weight4(1.0 - f.y);

	//make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
	linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
	columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

	vec2 xystart = (-1.5 - f) * stepxy + pos;
	vec4 xpos = vec4(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0, xystart.x + stepxy.x * 3.0);


// final sum and weight normalization
   vec4 final = vec4(line_run(xystart.y                 , xpos, linetaps, Source) * columntaps.r +
                      line_run(xystart.y + stepxy.y      , xpos, linetaps, Source) * columntaps.g +
                      line_run(xystart.y + stepxy.y * 2.0, xpos, linetaps, Source) * columntaps.b +
                      line_run(xystart.y + stepxy.y * 3.0, xpos, linetaps, Source) * columntaps.a,1);

   FragColor = final;
}
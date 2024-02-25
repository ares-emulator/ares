#version 450

/*
   Gaussian blur - dynamic range, resizable
   
   Copyright (C) 2020 guest(r) - guest.r@gmail.com

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
	float SIZEH;
	float SIGMA_H;
	float SIZEV;
	float SIGMA_V;
} params;

#pragma parameter SIZEH "Horizontal Blur Radius" 6.0 0.0 25.0 1.0 
#define SIZEH params.SIZEH

#pragma parameter SIGMA_H "Horizontal Blur Sigma" 1.50 0.1 12.0 0.05
#define SIGMA_H params.SIGMA_H

#pragma parameter SIZEV "Vertical Blur Radius" 6.0 0.0 25.0 1.0 
#define SIZEV params.SIZEV

#pragma parameter SIGMA_V "Vertical Blur Sigma" 1.50 0.1 12.0 0.05
#define SIGMA_V params.SIGMA_V

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

#define COMPAT_TEXTURE(c,d) texture(c,d)
#define SourceSize params.SourceSize

float invsqrsigma_h = 1.0/(2.0*SIGMA_H*SIGMA_H);
float invsqrsigma_v = 1.0/(2.0*SIGMA_V*SIGMA_V);

float gaussian_h(float x)
{
	return exp(-x*x*invsqrsigma_h);
}

float gaussian_v(float x)
{
	return exp(-x*x*invsqrsigma_v);
}

void main()
{
	vec2 f = fract(SourceSize.xy * vTexCoord.xy);
	f = 0.5 - f;
	vec2 tex = floor(SourceSize.xy * vTexCoord)*SourceSize.zw + 0.5*SourceSize.zw;
	vec2 dx  = vec2(SourceSize.z, 0.0);	
	vec2 dy  = vec2(0.0, SourceSize.w);

	vec3 colorx = vec3(0.0);
	vec3 colory = vec3(0.0);
	float wx, wy;
	float wsumx;
	float wsumy = 0.0;
	vec3 pixel;
	float x;
	float y = -SIZEV;

	do
	{
		wsumx  = 0.0;
		colorx = 0.0.xxx;
		x = -SIZEH;
		
		do
		{
			pixel  = COMPAT_TEXTURE(Source, tex + x*dx + y*dy).rgb;
			wx     = gaussian_h(x+f.x);
			colorx = colorx + wx * pixel;
			wsumx  = wsumx+ wx;
			x = x + 1.0;
			
		} while (x <= SIZEH);
		
		colorx  = colorx / wsumx;
		wy      = gaussian_v(y+f.y);
		colory  = colory + wy * colorx;
		wsumy   = wsumy + wy;
		y = y + 1.0;
		
	} while (y <= SIZEV);

	colory= colory/wsumy;
	
	FragColor = vec4(colory, 1.0);
}

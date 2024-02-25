#version 450

/*
   Gaussian blur - vertical pass, dynamic range, resizable
   
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
	float SIZEV;
	float SIGMA_V;
} params;


#pragma parameter SIZEV "Vertical Blur Radius" 5.0 0.0 40.0 1.0 
#define SIZEV params.SIZEV

#pragma parameter SIGMA_V "Vertical Blur Sigma" 1.0 0.1 15.0 0.05
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

float invsqrsigma = 1.0/(2.0*SIGMA_V*SIGMA_V);

float gaussian(float x)
{
	return exp(-x*x*invsqrsigma);
}

void main()
{
	float f = fract(SourceSize.y * vTexCoord.y);
	f = 0.5 - f;
	vec2 tex = floor(SourceSize.xy * vTexCoord)*SourceSize.zw + 0.5*SourceSize.zw;
	vec3 color = vec3(0.0);
	vec2 dy  = vec2(0.0, SourceSize.w);

	float w;
	float wsum = 0.0;
	vec3 pixel;
	float n = -SIZEV;

	do
	{
		pixel  = COMPAT_TEXTURE(Source, tex + n*dy).rgb;
		w      = gaussian(n+f);
		color  = color + w * pixel;
		wsum   = wsum + w;
		n = n + 1.0;
		
	} while (n <= SIZEV);

	color = color / wsum;
	
	FragColor = vec4(color, 1.0);
}
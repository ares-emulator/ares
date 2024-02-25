#version 450

/*
   G-sharp resampler - dynamic range, resizable
   
   Copyright (C) 2020 - 2021 guest(r) - guest.r@gmail.com

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

   Anti-Ringing inspired by Hyllian
   
*/ 

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float SIGMA_HV;
	float HSHARP0;
	float HAR;
	float SHAR;
} params;

#pragma parameter HSHARP0 "Filter Range" 1.2 1.0 6.0 0.1
#define HSHARP0 params.HSHARP0

#pragma parameter SIGMA_HV "Gaussian Blur Sigma" 0.75 0.1 7.0 0.05
#define SIGMA_HV params.SIGMA_HV

#pragma parameter SHAR "Sharpness Definition" 0.5 0.0 2.0 0.05
#define SHAR params.SHAR

#pragma parameter HAR "Anti-Ringing" 0.5 0.0 1.0 0.10
#define HAR params.HAR

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

float invsqrsigma_h = 1.0/(2.0*SIGMA_HV*SIGMA_HV);

float gaussian(float x, float y)
{
	return exp(-(x*x + y*y)*invsqrsigma_h);
}

void main()
{
	vec2 f = fract(SourceSize.xy * vTexCoord.xy);
	f = 0.5 - f;
	vec2 tex = floor(SourceSize.xy * vTexCoord)*SourceSize.zw + 0.5*SourceSize.zw;
	vec2 dx  = vec2(SourceSize.z, 0.0);	
	vec2 dy  = vec2(0.0, SourceSize.w);

	vec3 colorx = 0.0.xxx;
	vec3 colory = 0.0.xxx;
	float wx, wy;
	float wsumx = 0.0;
	float wsumy = 0.0;
	vec3 pixel;
	float x;

	vec3 xcmax = 0.0.xxx;
	vec3 xcmin = 1.0.xxx;

	float sharp = gaussian(HSHARP0, 0.0);
	float maxsharp = 0.07;
	float FPR = HSHARP0;
	float fpx = 1.0;

	float LOOPSIZE = ceil(2.0*FPR);
	float y = -LOOPSIZE;

	do
	{
		x = -LOOPSIZE;
	
		do
		{
			pixel  = COMPAT_TEXTURE(Source, tex + x*dx + y*dy).rgb;
			
			wx = gaussian(x+f.x, y+f.y) - sharp; 
			fpx = (sqrt(dot(vec2(x+f.x,y+f.y),vec2(x+f.x,y+f.y)))-FPR)/FPR;
			if (((x*x) + (y*y)) < 1.25*FPR) { xcmax = max(xcmax, pixel); xcmin = min(xcmin, pixel); }
			if (wx < 0.0) wx = clamp(wx, mix(-maxsharp, 0.0, pow(abs(fpx), SHAR)), 0.0);
			colorx = colorx + wx * pixel;
			wsumx  = wsumx + wx;
			x = x + 1.0;
			
		} while (x <= LOOPSIZE);
		
		y = y + 1.0;
		
	} while (y <= LOOPSIZE);

	vec3 color = colorx/wsumx;
	
	color = mix(clamp(color, 0.0, 1.0), clamp(color, xcmin, xcmax), HAR);
	
	FragColor = vec4(color, 1.0);
}

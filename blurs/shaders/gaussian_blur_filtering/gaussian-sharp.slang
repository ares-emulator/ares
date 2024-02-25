#version 450

/*
   Gaussian blur 'sharp'- dynamic range, resizable
   
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

   Anti-Ringing inspired by Hyllian
   
*/ 

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float SIZEH;
	float SIGMA_H;
	float HSHARP0;
	float HAR;
	float SHAR;
} params;

#pragma parameter SIZEH "Gaussian Blur Radius" 6.0 0.0 25.0 1.0 
#define SIZEH params.SIZEH

#pragma parameter SIGMA_H "Gaussian Blur Sigma" 1.50 0.1 12.0 0.05
#define SIGMA_H params.SIGMA_H

#pragma parameter HSHARP0 "Sharpness Filter 'Range'" 0.8 0.8 20.0 0.2
#define HSHARP0 params.HSHARP0

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

float invsqrsigma_h = 1.0/(2.0*SIGMA_H*SIGMA_H);

float gaussian_h(float x)
{
	return exp(-x*x*invsqrsigma_h);
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
	float y = -SIZEH;

	vec3 xcmax = COMPAT_TEXTURE(Source, tex).rgb;
	vec3 xcmin = xcmax;
	vec3 ycmax = xcmax;
	vec3 ycmin = xcmax;	
	float sharp = (HSHARP0 > 0.85) ? gaussian_h(HSHARP0) : 0.0;
	float FPR = HSHARP0;
	float wnorm = 1.0; // /(1.0-sharp);
	float fpx = 0.0;

	do
	{
		wsumx  = 0.0;
		colorx = 0.0.xxx;
		x = -SIZEH;
		
		do
		{
			pixel  = COMPAT_TEXTURE(Source, tex + x*dx + y*dy).rgb;
			wx = (gaussian_h(x+f.x) - sharp)*wnorm; 
			
			if (HSHARP0 > 0.85)
			{
				if (abs(x+f.x) > FPR) { if (x < 0.0) fpx = -(x+f.x+FPR)/FPR; else fpx = (x+f.x-FPR)/FPR; fpx = min(fpx, 1.0); }
				if (abs(x) <= 2.0*FPR) { xcmax = max(xcmax, pixel); xcmin = min(xcmin, pixel); }
				if (wx < 0.0) wx = clamp(wx, mix(-0.18, 0.0, pow(fpx, SHAR)), 0.0);
			} 			
			
			colorx = colorx + wx * pixel;
			wsumx  = wsumx+ wx;
			x = x + 1.0;
			
		} while (x <= SIZEH);
		
		colorx  = colorx / wsumx;
		if (HSHARP0 > 0.85) colorx = mix(clamp(colorx, 0.0, 1.0), clamp(colorx, xcmin, xcmax), HAR); 
		
		wy = (gaussian_h(y+f.y) - sharp)*wnorm; 
		
		if (HSHARP0 > 0.85)
		{
			if (abs(y+f.y) > FPR) { if (y < 0.0) fpx = -(y+f.y+FPR)/FPR; else fpx = (y+f.y-FPR)/FPR; fpx = min(fpx, 1.0); }
			if (abs(y) <= 2.0*FPR) { ycmax = max(ycmax, colorx); ycmin = min(ycmin, colorx); }
			if (wy < 0.0) wy = clamp(wy, mix(-0.18, 0.0, pow(fpx, SHAR)), 0.0);
		} 					
		
		colory  = colory + wy * colorx;
		wsumy   = wsumy + wy;
		y = y + 1.0;
		
	} while (y <= SIZEH);

	colory= colory/wsumy;
	if (HSHARP0 > 0.85) colory = mix(clamp(colory, 0.0, 1.0), clamp(colory, ycmin, ycmax), HAR);
		
	FragColor = vec4(colory, 1.0);
}

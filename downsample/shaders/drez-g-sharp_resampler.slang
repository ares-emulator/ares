#version 450

/*
   drez-g-sharp_resampler.slang
   DREZ Resolution Downsampler, This is really just a wrapper around Guest's great G-sharp resampler
   General usage is to add this as a pass with a reduced resolution

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

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float DREZ_GSHARP_ON;
	float DREZ_SIGMA_HV;
	float DREZ_HSHARP0;
	float DREZ_HAR;
	float DREZ_SHAR;
	float DREZ_THRESHOLD_RATIO;
} global;

#pragma parameter GSHARP_DREZ_EMPTY_LINE		" " 0 0 0.001 0.001
#pragma parameter GSHARP_DREZ_TITLE 	"[ --- DREZ DOWNSAMPLE FILTER - GUEST.R G-SHARP RESAMPLER --- ]:"  0 0 0.01 0.01

#pragma parameter DREZ_GSHARP_ON 	"          G-SHARP ON" 	  1 0 1 1
#define DREZ_GSHARP_ON global.DREZ_GSHARP_ON

#pragma parameter DREZ_THRESHOLD_RATIO 	"         Downsample Threshold (Original Res Vs Downsampled Res)" 	  1.5 1 4 0.1
#define DREZ_THRESHOLD_RATIO global.DREZ_THRESHOLD_RATIO


// Default was 1.2, now set to 2.3 to match smoothing in Hyllian b-spline
#pragma parameter DREZ_HSHARP0 		"          Filter Range"  2.3 1.0 6.0 0.1
#define DREZ_HSHARP0 global.DREZ_HSHARP0

// Default was 0.75
#pragma parameter DREZ_SIGMA_HV 		"          Gaussian Blur Sigma" 0.75 0.1 7.0 0.05
#define DREZ_SIGMA_HV global.DREZ_SIGMA_HV

// Default was 0.5
#pragma parameter DREZ_SHAR 			"          Sharpness Definition" 0.5 0.0 2.0 0.05
#define DREZ_SHAR global.DREZ_SHAR

// Default was 0.5
#pragma parameter DREZ_HAR 			"          Anti-Ringing" 0.5 0.0 1.0 0.10
#define DREZ_HAR global.DREZ_HAR


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
layout(set = 0, binding = 2) uniform sampler2D Original;

#define COMPAT_TEXTURE(c,d) texture(c,d)
#define SourceSize global.OriginalSize
#define OutputSize global.OutputSize
#define Source Original

float invsqrsigma_h = 1.0/(2.0*DREZ_SIGMA_HV*DREZ_SIGMA_HV);

float gaussian(float x, float y)
{
	return exp(-(x*x + y*y)*invsqrsigma_h);
}

void main()
{
	// If the effext is disabled or the downsample ratio is below the threshold, just sample the texture
	if (DREZ_GSHARP_ON == 0 || SourceSize.y / OutputSize.y < DREZ_THRESHOLD_RATIO)
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}
	
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

	float sharp = gaussian(DREZ_HSHARP0, 0.0);
	float maxsharp = 0.07;
	float FPR = DREZ_HSHARP0;
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
			if (wx < 0.0) wx = clamp(wx, mix(-maxsharp, 0.0, pow(abs(fpx), DREZ_SHAR)), 0.0);
			colorx = colorx + wx * pixel;
			wsumx  = wsumx + wx;
			x = x + 1.0;
			
		} while (x <= LOOPSIZE);
		
		y = y + 1.0;
		
	} while (y <= LOOPSIZE);

	vec3 color = colorx/wsumx;
	
	color = mix(clamp(color, 0.0, 1.0), clamp(color, xcmin, xcmax), DREZ_HAR);
	
	FragColor = vec4(color, 1.0);
}

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
	float HSM_SCREEN_SCALE_GSHARP_MODE;
	float HSM_SCREEN_SCALE_SIGMA_HV;
	float HSM_SCREEN_SCALE_HSHARP0;
	float HSM_SCREEN_SCALE_HAR;
	float HSM_SCREEN_SCALE_SHAR;
	float HSM_SCREEN_SCALE_THRESHOLD_RATIO;
} params;

#pragma parameter HSM_GSHARP_SCREEN_SCALE_EMPTY_LINE		" " 0 0 0.001 0.001
#pragma parameter GSHARP_SCREEN_SCALE_TITLE 	"[ --- SCREEN SCALE G-SHARP RESAMPLER by Guest.r --- ]:"  0 0 0.01 0.01

#pragma parameter HSM_SCREEN_SCALE_GSHARP_MODE 	"          G-SHARP MODE: OFF | SHARP | EDITABLE" 	  1 0 2 1
#define HSM_SCREEN_SCALE_GSHARP_MODE params.HSM_SCREEN_SCALE_GSHARP_MODE

// Default was 1.2, now set to 2.3 to match smoothing in Hyllian b-spline
#pragma parameter HSM_SCREEN_SCALE_HSHARP0 		"          Filter Range"  1.0 1.0 6.0 0.1
#define HSM_SCREEN_SCALE_HSHARP0 (HSM_SCREEN_SCALE_GSHARP_MODE == 2 ? params.HSM_SCREEN_SCALE_HSHARP0 : 1)

// Default was 0.75
#pragma parameter HSM_SCREEN_SCALE_SIGMA_HV 	"          Gaussian Blur Sigma" 0.25 0.1 7.0 0.05
#define HSM_SCREEN_SCALE_SIGMA_HV (HSM_SCREEN_SCALE_GSHARP_MODE == 2 ? params.HSM_SCREEN_SCALE_SIGMA_HV : 0.1)

// Default was 0.5
#pragma parameter HSM_SCREEN_SCALE_SHAR 		"          Sharpness Definition" 0 0.0 2.0 0.05
#define HSM_SCREEN_SCALE_SHAR params.HSM_SCREEN_SCALE_SHAR

// Default was 0.5
#pragma parameter HSM_SCREEN_SCALE_HAR 			"          Anti-Ringing" 0.5 0.0 1.0 0.10
#define HSM_SCREEN_SCALE_HAR params.HSM_SCREEN_SCALE_HAR


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
layout(set = 0, binding = 3) uniform sampler2D InfoCachePass;
// layout(set = 0, binding = 4) uniform sampler2D AvgLumPass;

// HSM Removed
// #define COMPAT_TEXTURE(c,d) texture(c,d)

// HSM Added
#define COMPAT_TEXTURE(c,d) HSM_GetCroppedTexSample(c,d)
// End Addition

#define SourceSize params.SourceSize
#define OutputSize params.OutputSize

float invsqrsigma_h = 1.0/(2.0*HSM_SCREEN_SCALE_SIGMA_HV*HSM_SCREEN_SCALE_SIGMA_HV);

float gaussian(float x, float y)
{
	return exp(-(x*x + y*y)*invsqrsigma_h);
}

void main()
{
    // HSM Added
	// vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	vec2 cache_bounds_coord = SCREEN_COORD;

// If it's the potato preset render the whole frame
#ifndef IS_POTATO_PRESET
#ifndef IS_NO_REFLECT_PRESET
	// Have to get the scale of the coordinates so we can figure out the size of the onscreen rectangle of the area 
	HSM_GetBezelCoords(TUBE_DIFFUSE_COORD, 
						TUBE_DIFFUSE_SCALE, 
						TUBE_SCALE, 
						SCREEN_ASPECT, 
						false,
						BEZEL_OUTSIDE_SCALE,
						BEZEL_OUTSIDE_COORD, 
						BEZEL_OUTSIDE_CURVED_COORD, 
						FRAME_OUTSIDE_CURVED_COORD);
	cache_bounds_coord = (BEZEL_OUTSIDE_COORD - 0.5) * 0.9 + 0.5;
#endif
#endif

	if (HHLP_IsOutsideCoordSpace(cache_bounds_coord))
	{
		FragColor = vec4(0);
		return;
	}

	// HSM Added
	float raster_bloom_avg_lum = 0.5; //texture(AvgLumPass, vec2(0.5,0.5)).a;
	vec2 screen_curved_coord = vec2(0.5);

	vec2 screen_curved_coord_with_overscan_and_mirror = HSM_GetCrtShaderFinalCoord(SCREEN_COORD,
		SCREEN_SCALE,
		raster_bloom_avg_lum,
		screen_curved_coord);

	vec2 curved_coord = screen_curved_coord_with_overscan_and_mirror;

    // End Addition
	
	if (HSM_SCREEN_SCALE_GSHARP_MODE == 0)
	{
		FragColor = texture(Source, curved_coord);
		return;
	}

	vec2 f = fract(SourceSize.xy * curved_coord.xy);
	f = 0.5 - f;
	vec2 tex = floor(SourceSize.xy * curved_coord)*SourceSize.zw + 0.5*SourceSize.zw;
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

	float sharp = gaussian(HSM_SCREEN_SCALE_HSHARP0, 0.0);
	float maxsharp = 0.07;
	float FPR = HSM_SCREEN_SCALE_HSHARP0;
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
			if (wx < 0.0) wx = clamp(wx, mix(-maxsharp, 0.0, pow(abs(fpx), HSM_SCREEN_SCALE_SHAR)), 0.0);
			colorx = colorx + wx * pixel;
			wsumx  = wsumx + wx;
			x = x + 1.0;
			
		} while (x <= LOOPSIZE);
		
		y = y + 1.0;
		
	} while (y <= LOOPSIZE);

	vec3 color = colorx/wsumx;
	
	color = mix(clamp(color, 0.0, 1.0), clamp(color, xcmin, xcmax), HSM_SCREEN_SCALE_HAR);
	
	FragColor = vec4(color, 1.0);
}

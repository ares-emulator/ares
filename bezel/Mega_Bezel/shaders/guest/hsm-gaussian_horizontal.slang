#version 450

/*
   Gaussian blur - horizontal pass, dynamic range, resizable
   
   Copyright (C) 2020 - 2023 guest(r) - guest.r@gmail.com

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
	vec4 DerezedPassSize;
	float SIZEH;
	float SIGMA_H;
	float FINE_GLOW;	
	float m_glow;
	float m_glow_cutoff;
	float m_glow_low;
	float m_glow_high;
	float m_glow_dist;
	float m_glow_mask;
} params;

#pragma parameter GDV_GDV_ADV_LINE							" " 0 0 0.001 0.001
#pragma parameter GDV_GDV_ADV_TITLE_2 		"[ --- GUEST-DrVenom Advanced --- ]:" 0 0 0.001 0.001

#pragma parameter bogus_mglow "[ MAGIC GLOW SETTINGS ]:" 0 0 0.001 0.001

#pragma parameter FINE_GLOW "          Fine Glow/M.Glow Sampling" 1.0 1.0 5.0 1.0 
#define FINE_GLOW params.FINE_GLOW

#pragma parameter m_glow 		"          Ordinary Glow / Magic Glow -- m_glow" 0.0 0.0 1.0 1.0
#define m_glow params.m_glow

#pragma parameter m_glow_cutoff "          Magic Glow Cutoff -- m_glow_cutoff" 0.12 0.0 0.4 0.01 
#define m_glow_cutoff params.m_glow_cutoff

#pragma parameter m_glow_low 	"          Magic Glow Low Strength -- m_glow_low" 0.35 0.0 7.0 0.05 

#pragma parameter m_glow_high 	"          Magic Glow High Strength -- m_glow_high" 5.0 0.0 7.0 0.10 

#pragma parameter m_glow_dist 	"          Magic Glow Distribution -- m_glow_dist" 1.0 0.20 4.0 0.05 

#pragma parameter m_glow_mask 	"          Magic Glow Mask Strength -- m_glow_mask" 1.0 0.0 2.0 0.025 

#pragma parameter bogus_glow "[ GLOW PASS SETTINGS ]:" 0 0 0.001 0.001

#pragma parameter SIZEH 		"          Horizontal Glow Radius (Num Samples) -- SIZEH" 6.0 1.0 50.0 1.0 
#define SIZEH params.SIZEH

#pragma parameter SIGMA_H 		"          Horizontal Glow Sigma ( Spread ) -- SIGMA_H" 1.20 0.20 15.0 0.05
#define SIGMA_H params.SIGMA_H


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
layout(set = 0, binding = 2) uniform sampler2D LinearizePass;

#define COMPAT_TEXTURE(c,d) texture(c,d)

float invsqrsigma = 1.0/(2.0*SIGMA_H*SIGMA_H);

float gaussian(float x)
{
	return exp(-x*x*invsqrsigma);
}

vec3 plant (vec3 tar, float r)
{
	float t = max(max(tar.r,tar.g),tar.b) + 0.00001;
	return tar * r / t;
}

void main()
{
	vec4 SourceSize1 = params.DerezedPassSize*mix(1.0.xxxx, vec4(FINE_GLOW, FINE_GLOW, 1.0/FINE_GLOW, 1.0/FINE_GLOW), min(FINE_GLOW-1.0,1.0));
	float f = fract(SourceSize1.x * vTexCoord.x);
	f = 0.5 - f;
	vec2 tex = floor(SourceSize1.xy * vTexCoord)*SourceSize1.zw + 0.5*SourceSize1.zw;
	vec3 color = vec3(0.0);
	vec2 dx  = vec2(SourceSize1.z, 0.0);

	float w;
	float wsum = 0.0;
	vec3 pixel;
	float n = -SIZEH;

	do
	{
		pixel  = COMPAT_TEXTURE(LinearizePass, tex + n*dx).rgb;
		if (m_glow > 0.5)
		{
			pixel = max(pixel-m_glow_cutoff, 0.0);
			pixel = plant(pixel, max(max(max(pixel.r,pixel.g),pixel.b)-m_glow_cutoff,0.0));
		}
		w      = gaussian(n+f);
		color  = color + w * pixel;
		wsum   = wsum + w;
		n = n + 1.0;
		
	} while (n <= SIZEH);

	color = color / wsum;
	
	FragColor = vec4(color, 1.0);
}
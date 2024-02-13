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
	float SIZEHB;
	float SIGMA_HB;
	float FINE_BLOOM;
} params;

#pragma parameter GDV_BLOOM_LINE							" " 0 0 0.001 0.001
#pragma parameter bogus_bloom "[ BLOOM/HALATION/(GLOW) PASS SETTINGS ]:" 0 0 0.001 0.001

#pragma parameter FINE_BLOOM "          Fine Bloom/Halation Sampling" 1.0 1.0 4.0 1.0 
#define FINE_BLOOM params.FINE_BLOOM

#pragma parameter SIZEHB 		"          Horizontal Bloom/Halation Radius -- SIZEHB" 3.0 1.0 50.0 1.0 
#define SIZEHB params.SIZEHB

#pragma parameter SIGMA_HB 		"         Horizontal Bloom/Halation Sigma -- SIGMA_HB" 0.75 0.25 15.0 0.025
#define SIGMA_HB params.SIGMA_HB

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

float invsqrsigma = 1.0/(2.0*SIGMA_HB*SIGMA_HB);

float gaussian(float x)
{
	return exp(-x*x*invsqrsigma);
}

void main()
{
	vec4 SourceSize1 = params.DerezedPassSize * mix(1.0.xxxx, vec4(FINE_BLOOM, FINE_BLOOM, 1.0/FINE_BLOOM, 1.0/FINE_BLOOM), min(FINE_BLOOM-1.0,1.0));
	float f = fract(SourceSize1.x * vTexCoord.x);
	f = 0.5 - f;
	vec2 tex = floor(SourceSize1.xy * vTexCoord)*SourceSize1.zw + 0.5*SourceSize1.zw;
	vec4 color = vec4(0.0);
	vec2 dx  = vec2(SourceSize1.z, 0.0);

	float w;
	float wsum = 0.0;
	vec4 pixel;
	float n = -SIZEHB;

	do
	{
		pixel  = COMPAT_TEXTURE(LinearizePass, tex + n*dx);
		w = gaussian(n+f);
		pixel.a = max(max(pixel.r, pixel.g),pixel.b);
		pixel.a*=pixel.a*pixel.a;
		color  = color + w * pixel;
		wsum   = wsum + w;
		n = n + 1.0;
		
	} while (n <= SIZEHB);

	color = color / wsum;
	
	FragColor = vec4(color.rgb, pow(color.a, 0.333333));
}
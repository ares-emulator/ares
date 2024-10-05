#version 450

/*
   CRT - Guest - Advanced - Fast - Pass1
   
   Copyright (C) 2018-2023 guest(r) - guest.r@gmail.com

   Incorporates many good ideas and suggestions from Dr. Venom.
   I would also like give thanks to many Libretro forums members for continuous feedback, suggestions and caring about the shader.
   
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
	float IOS, h_sharp, s_sharp, spike, ring;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;


#pragma parameter bogus_filtering "[ FILTERING OPTIONS ]: " 0.0 0.0 1.0 1.0

#pragma parameter h_sharp "          Horizontal sharpness" 5.20 0.20 15.0 0.10
#define h_sharp      params.h_sharp     // pixel sharpness

#pragma parameter s_sharp "          Substractive sharpness (1.0 recommended)" 0.50 0.0 2.0 0.10
#define s_sharp      params.s_sharp     // substractive sharpness

#pragma parameter ring "          Substractive sharpness Ringing" 0.0 0.0 3.0 0.05
#define ring      params.ring     // substractive sharpness ringing

#pragma parameter spike "          Scanline Spike Removal" 1.0 0.0 2.0 0.10
#define spike params.spike

#define COMPAT_TEXTURE(c,d) texture(c,d)
#define TEX0 vTexCoord

#define SourceSize params.OriginalSize
#define OutputSize params.OutputSize
#define gl_FragCoord (vTexCoord * OutputSize.xy)

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.00001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D LinearizePass;


void main()
{
	float intera = COMPAT_TEXTURE(LinearizePass, vec2(0.75,0.25)).a;
	
	// Calculating texel coordinates
   
	vec2 texcoord = TEX0.xy;
   
	vec2 pos  = texcoord;

	vec2 coffset = vec2(0.5, 0.5);
	
	vec2 ps = SourceSize.zw;
	vec2 OGL2Pos = pos * SourceSize.xy - coffset;
	float fpx = fract(OGL2Pos.x);
	
	vec2 offx = vec2(ps.x,0.0);
   
	// Reading the texels
	vec2 x2 = 2.0*offx;
	
	vec2 pC4 = floor(OGL2Pos) * ps + 0.5*ps; 
	
	float zero = exp2(-h_sharp);
	float sharp1 = s_sharp * zero;
	
	float wl2 = 1.0 + fpx;
	float wl1 =       fpx;
	float wr1 = 1.0 - fpx;
	float wr2 = 2.0 - fpx;

	wl2*=wl2; wl2 = exp2(-h_sharp*wl2);
	wl1*=wl1; wl1 = exp2(-h_sharp*wl1);
	wr1*=wr1; wr1 = exp2(-h_sharp*wr1);
	wr2*=wr2; wr2 = exp2(-h_sharp*wr2);
	
	float fp1 = 1.-fpx;

	float twl2 = max(wl2 - sharp1, mix(-0.12, 0.0, 1.0-fp1*fp1));
	float twl1 = max(wl1 - sharp1, -0.12);
	float twr1 = max(wr1 - sharp1, -0.12);
	float twr2 = max(wr2 - sharp1, mix(-0.12, 0.0, 1.0-fpx*fpx));
	
	bool sharp = (sharp1 > 0.0);
	
	vec3 l2, l1, r1, r2, color1, colmin, colmax;

	l2 = COMPAT_TEXTURE(LinearizePass, pC4 -offx).rgb;
	l1 = COMPAT_TEXTURE(LinearizePass, pC4      ).rgb;
	r1 = COMPAT_TEXTURE(LinearizePass, pC4 +offx).rgb;
	r2 = COMPAT_TEXTURE(LinearizePass, pC4   +x2).rgb;

	colmin = min(min(l1,r1), min(l2,r2));
	colmax = max(max(l1,r1), max(l2,r2));
	
	color1 = (l2*twl2 + l1*twl1 + r1*twr1 + r2*twr2)/(twl2+twl1+twr1+twr2);

	if (sharp) color1 = clamp(mix(clamp(color1, colmin, colmax), color1, ring), 0.0, 1.0);
	float ts = 0.025;
	vec3 luma = vec3(0.2126, 0.7152, 0.0722);
	
	float lm2 = max(max(l2.r,l2.g),l2.b);
	float lm1 = max(max(l1.r,l1.g),l1.b);
	float rm1 = max(max(r1.r,r1.g),r1.b);
	float rm2 = max(max(r2.r,r2.g),r2.b);	
	
	float swl2 = max(twl2, 0.0) * (dot(l2,luma) + ts);
	float swl1 = max(twl1, 0.0) * (dot(l1,luma) + ts);
	float swr1 = max(twr1, 0.0) * (dot(r1,luma) + ts);
	float swr2 = max(twr2, 0.0) * (dot(r2,luma) + ts);
	
	float fscolor1 = (lm2*swl2 + lm1*swl1 + rm1*swr1 + rm2*swr2)/(swl2+swl1+swr1+swr2);
	float sresult = clamp(mix(max(max(color1.r,color1.g),color1.b), fscolor1, spike), 0.0, 1.0);
	
	FragColor = vec4(color1, sresult);
} 
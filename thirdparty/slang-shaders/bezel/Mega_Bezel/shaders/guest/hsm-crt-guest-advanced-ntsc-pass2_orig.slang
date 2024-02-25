#version 450

/*
   CRT - Guest - Advanced - NTSC
   
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
	float IOS, OS, BLOOM, brightboost, brightboost1, gsl, scanline1, scanline2, beam_min, beam_max, beam_size,
	h_sharp, s_sharp, warpX, warpY, glow, shadowMask, masksize, ring, no_scanlines, tds, clips, ssharp, hiscan;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float bloom;
	float halation;
	float scans;
	float gamma_c;
	float gamma_out;
	float overscanX;
	float overscanY;
	float intres;
	float prescalex;
	float c_shape;
	float scangamma;
	float sborder;
	float scan_falloff;
	float bloom_dist;
	float bmask1;
	float hmask1;
} global;

#pragma parameter hiscan "          High Resolution Scanlines (prepend a scaler...)" 0.0 0.0 1.0 1.0
#define hiscan        params.hiscan

#pragma parameter bogus_brightness "[ BRIGHTNESS SETTINGS ]:" 0.0 0.0 1.0 1.0

#pragma parameter glow "          (Magic) Glow Strength" 0.08 -2.0 2.0 0.01
#define glow         params.glow     // Glow Strength

#pragma parameter bloom "          Bloom Strength" 0.0 -2.0 2.0 0.05
#define bloom         global.bloom     // bloom effect

#pragma parameter mask_bloom "          Mask Bloom" 0.0 -2.0 2.0 0.05
#define mask_bloom         params.mask_bloom     // bloom effect

#pragma parameter bloom_dist "          Bloom Distribution" 0.0 -2.0 3.0 0.05
#define bloom_dist         global.bloom_dist     // bloom effect distribution

#pragma parameter halation "          Halation Strength" 0.0 -2.0 2.0 0.025
#define halation         global.halation     // halation effect

#pragma parameter bmask1 "          Bloom Mask Strength" 0.0 -1.0 1.0 0.025
#define bmask1         global.bmask1     // bloom/halation mask strength

#pragma parameter hmask1 "          Halation Mask Strength" 0.5 -1.0 1.0 0.025
#define hmask1         global.hmask1     // bloom/halation mask strength

#pragma parameter gamma_c "          Gamma correct" 1.0 0.50 2.0 0.025
#define gamma_c   global.gamma_c     // adjust brightness

#pragma parameter brightboost "          Bright Boost Dark Pixels" 1.40 0.25 10.0 0.05
#define brightboost  params.brightboost     // adjust brightness

#pragma parameter brightboost1 "          Bright Boost Bright Pixels" 1.10 0.25 3.00 0.025
#define brightboost1  params.brightboost1     // adjust brightness

#pragma parameter clips "          Clip Saturated Color Beams" 0.0 -1.0 1.0 0.05
#define clips  params.clips     // kinky effect

#pragma parameter bogus_scanline "[ SCANLINE OPTIONS ]: " 0.0 0.0 1.0 1.0

#pragma parameter gsl "          Scanline Type" 0.0 -1.0 2.0 1.0
#define gsl          params.gsl      // Alternate scanlines

#pragma parameter scanline1 "          Scanline Beam Shape Center" 6.0 -20.0 40.0 0.5
#define scanline1    params.scanline1      // scanline param, vertical sharpness

#pragma parameter scanline2 "          Scanline Beam Shape Edges" 8.0 0.0 70.0 1.0 
#define scanline2    params.scanline2      // scanline param, vertical sharpness

#pragma parameter beam_min "          Scanline Shape Dark Pixels" 1.30 0.25 10.0 0.05
#define beam_min     params.beam_min     // dark area beam min - narrow

#pragma parameter beam_max "          Scanline Shape Bright Pixels" 1.00 0.2 3.5 0.025
#define beam_max     params.beam_max     // bright area beam max - wide

#pragma parameter tds "          Thinner Dark Scanlines" 0.0 0. 1.0 1.0
#define tds     params.tds     // thinner dark scanlines

#pragma parameter beam_size "          Increased Bright Scanline Beam" 0.60 0.0 1.0 0.05
#define beam_size    params.beam_size     // increased max. beam size

#pragma parameter scans "          Scanline Saturation / Mask Falloff" 0.50 0.0 6.0 0.10
#define scans        1.5*global.scans     // scanline saturation

#pragma parameter scan_falloff "          Scanline Falloff" 1.0 0.10 2.0 0.025
#define scan_falloff        global.scan_falloff     // scanline falloff

#pragma parameter ssharp "          Smart Sharpen Scanlines" 0.0 0.0 0.30 0.01
#define ssharp params.ssharp

#pragma parameter scangamma "          Scanline Gamma" 2.40 0.5 5.0 0.05
#define scangamma global.scangamma

#pragma parameter no_scanlines "          No-scanline mode" 0.0 0.0 1.5 0.05
#define no_scanlines params.no_scanlines

#pragma parameter bogus_screen "[ SCREEN OPTIONS ]: " 0.0 0.0 1.0 1.0

#pragma parameter intres "          Internal Resolution Y: 0.5...y-dowsample" 0.0 0.0 6.0 0.5 // Joint parameter with linearize pass, values must match

#pragma parameter IOS "          Integer Scaling: Odd:Y, Even:'X'+Y" 0.0 0.0 4.0 1.0
#define IOS          params.IOS     // Smart Integer Scaling

#pragma parameter OS "          R. Bloom Overscan Mode" 1.0 0.0 2.0 1.0
#define OS           params.OS     // Do overscan

#pragma parameter BLOOM "          Raster bloom %" 0.0 0.0 20.0 1.0
#define BLOOM        params.BLOOM     // Bloom overscan percentage

#pragma parameter warpX "          CurvatureX (default 0.03)" 0.0 0.0 0.25 0.01
#define warpX        params.warpX     // Curvature X

#pragma parameter warpY "          CurvatureY (default 0.04)" 0.0 0.0 0.25 0.01
#define warpY        params.warpY     // Curvature Y

#pragma parameter c_shape "          Curvature Shape" 0.25 0.05 0.60 0.05
#define c_shape        global.c_shape     // curvature shape

#pragma parameter overscanX "          Overscan X original pixels" 0.0 -200.0 200.0 1.0
#define overscanX        global.overscanX     // OverscanX pixels

#pragma parameter overscanY "          Overscan Y original pixels" 0.0 -200.0 200.0 1.0
#define overscanY        global.overscanY     // OverscanY pixels


#define COMPAT_TEXTURE(c,d) texture(c,d)
#define TEX0 vTexCoord

#define OutputSize global.OutputSize
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
layout(set = 0, binding = 3) uniform sampler2D AvgLumPass;
layout(set = 0, binding = 4) uniform sampler2D Pass1;

#define eps 1e-8

float st(float x)
{
	return exp2(-10.0*x*x);
} 

float st1(float x)
{
	return exp2(-8.0*x*x);
}
   
vec3 sw0(float x, float color, float scanline, vec3 c)
{
	float tmp = mix(beam_min, beam_max, color);
	vec3 sat = mix(1.0.xxx + scans, 1.0.xxx, c);
	float ex = x*tmp;
	ex = (gsl > -0.5) ? ex*ex : mix(ex*ex, ex*ex*ex, 0.4);
	return exp2(-scanline*ex*sat);
} 

vec3 sw1(float x, float color, float scanline, vec3 c)
{	
	x = mix (x, beam_min*x, max(x-0.4*color,0.0));
	vec3 sat = mix(1.0.xxx + scans, 1.0.xxx, c);	
	float tmp = mix(1.2*beam_min, beam_max, color);
	float ex = x*tmp;
	return exp2(-scanline*ex*ex*sat);
}    

vec3 sw2(float x, float color, float scanline, vec3 c)
{
	float tmp = mix((2.5-0.5*color)*beam_min, beam_max, color);
	vec3 sat = mix(1.0.xxx + scans, 1.0.xxx, c);	
	tmp = mix(beam_max, tmp, pow(x, color+0.3));
	float ex = x*tmp;
	return exp2(-scanline*ex*ex*sat);
}   

 
vec2 Warp(vec2 pos)
{
	pos  = pos*2.0-1.0;    
	pos  = mix(pos, vec2(pos.x*inversesqrt(1.0-c_shape*pos.y*pos.y), pos.y*inversesqrt(1.0-c_shape*pos.x*pos.x)), vec2(warpX, warpY)/c_shape);
	return pos*0.5 + 0.5;
}

vec2 Overscan(vec2 pos, float dx, float dy){
	pos=pos*2.0-1.0;    
	pos*=vec2(dx,dy);
	return pos*0.5+0.5;
} 


vec3 gc(vec3 c)
{
	float mc = max(max(c.r,c.g),c.b);
	float mg = pow(mc, 1.0/gamma_c);
	return c * mg/(mc + eps);  
}

vec3 plant (vec3 tar, float r)
{
	float t = max(max(tar.r,tar.g),tar.b) + 0.00001;
	return tar * r / t;
}


void main()
{
	float prescalex = float(textureSize(LinearizePass, 0).x) / (2.0*global.OriginalSize.x);
	
	vec4 SourceSize = global.OriginalSize * vec4(prescalex, 1.0, 1.0/prescalex, 1.0);
	
	SourceSize*= vec4(2.0, 1.0, 0.5, 1.0);
	
	float lum = COMPAT_TEXTURE(AvgLumPass, vec2(0.5,0.5)).a;
	
	float gamma_in = 1.0/COMPAT_TEXTURE(LinearizePass, vec2(0.25,0.25)).a;
	float intera = COMPAT_TEXTURE(LinearizePass, vec2(0.75,0.25)).a;
	bool hscan = (hiscan > 0.5);
	bool interb  = (((intera < 0.35) || (no_scanlines > 0.025)) && !hscan);

	float SourceY = SourceSize.y;	
	float sy = 1.0;
	if (global.intres == 1.0) sy = SourceY/224.0;
	if (global.intres > 0.25 && global.intres != 1.0) sy = global.intres;
	SourceSize*=vec4(1.0, 1.0/sy, 1.0, sy);
	
	// Calculating texel coordinates
   
	vec2 texcoord = TEX0.xy;
	if (IOS > 0.0 && !interb){
		vec2 ofactor = OutputSize.xy/global.OriginalSize.xy;
		vec2 intfactor = (IOS < 2.5) ? floor(ofactor) : ceil(ofactor);
		vec2 diff = ofactor/intfactor;
		float scan = diff.y;
		texcoord = Overscan(texcoord, scan, scan);
		if (IOS == 1.0 || IOS == 3.0) texcoord = vec2(TEX0.x, texcoord.y);
	}
   
	float factor  = 1.00 + (1.0-0.5*OS)*BLOOM/100.0 - lum*BLOOM/100.0;
	texcoord  = Overscan(texcoord, factor, factor);

	texcoord = Overscan(texcoord, (global.OriginalSize.x - overscanX)/global.OriginalSize.x, (global.OriginalSize.y - overscanY)/global.OriginalSize.y);

	vec2 pos  = Warp(texcoord);
	vec2 pos0 = Warp(TEX0.xy);

	vec2 coffset = vec2(0.5, 0.5);
	
	vec2 ps = SourceSize.zw;
	vec2 OGL2Pos = pos * SourceSize.xy - coffset;
	vec2 fp = fract(OGL2Pos);
	
	vec2 dx = vec2(ps.x,0.0);
	vec2 dy = vec2(0.0, ps.y);
   
	// Reading the texels

	float  f = fp.y;
	
	vec2 pC4 = floor(OGL2Pos) * ps + 0.5*ps;
	pC4.x = pos.x;

	if (global.intres == 0.5 && prescalex < 1.5) pC4.y = floor(pC4.y * global.OriginalSize.y)*global.OriginalSize.w + 0.5*global.OriginalSize.w;
	
	if (interb && no_scanlines < 0.025 || hscan)  pC4.y = pos.y; else if (interb) pC4.y = pC4.y + smoothstep(0.40-0.5*no_scanlines, 0.60 + 0.5*no_scanlines, f)*SourceSize.w;	
	
	vec3 color1 = COMPAT_TEXTURE(Pass1, pC4      ).rgb;
	vec3 scolor1 = 	COMPAT_TEXTURE(Pass1, pC4      ).aaa;

	if(!interb) color1 = pow(color1, vec3(scangamma/gamma_in));
  
	pC4+=dy;

	if (global.intres == 0.5 && prescalex < 1.5) pC4.y = floor((pos.y + 0.33*dy.y) * global.OriginalSize.y)*global.OriginalSize.w + 0.5*global.OriginalSize.w;
	
	vec3 color2 = COMPAT_TEXTURE(Pass1, pC4      ).rgb;
	vec3 scolor2 = 	COMPAT_TEXTURE(Pass1, pC4      ).aaa;

	if(!interb) color2 = pow(color2, vec3(scangamma/gamma_in)); 

	vec3 ctmp = color1; float w3 = 1.0; vec3 color = color1;
	vec3 one = vec3(1.0);
	
	if (hscan) { color2 = color1; scolor2 = scolor1; }

if (!interb || hscan)
{	
	// calculating scanlines
	vec3 luma = vec3(0.2126, 0.7152, 0.0722);
	float ssub = ssharp*max(abs(scolor1.x-scolor2.x), abs(dot(color1,luma)-dot(color2,luma)));

	float shape1 = mix(scanline1, scanline2 + ssub * scolor1.x * 35.0, f);
	float shape2 = mix(scanline1, scanline2 + ssub * scolor2.x * 35.0, 1.0-f);
	
	float wt1 = st(f);
	float wt2 = st(1.0-f);

	vec3 color00 = color1*wt1 + color2*wt2;
	vec3 scolor0 = scolor1*wt1 + scolor2*wt2;
	
	ctmp = color00/(wt1+wt2);
	vec3 sctmp = scolor0/(wt1+wt2);
	
	vec3 w1, w2;
	
	vec3 cref1 = mix(sctmp, scolor1, beam_size); float creff1 = pow(max(max(cref1.r,cref1.g),cref1.b), scan_falloff);
	vec3 cref2 = mix(sctmp, scolor2, beam_size); float creff2 = pow(max(max(cref2.r,cref2.g),cref2.b), scan_falloff);

	if (tds > 0.5) { shape1 = mix(scanline2, shape1, creff1); shape2 = mix(scanline2, shape2, creff2); }

	float f1 = f; 
	float f2 = 1.0-f;

	float mc1 = max(max(color1.r,color1.g),color1.b) + eps;
	float mc2 = max(max(color2.r,color2.g),color2.b) + eps;
	
	cref1 = color1 / mc1;
	cref2 = color2 / mc2;

	if (gsl <  0.5) { w1 = sw0(f1,creff1,shape1,cref1); w2 = sw0(f2,creff2,shape2,cref2);} else
	if (gsl == 1.0) { w1 = sw1(f1,creff1,shape1,cref1); w2 = sw1(f2,creff2,shape2,cref2);} else
	                { w1 = sw2(f1,creff1,shape1,cref1); w2 = sw2(f2,creff2,shape2,cref2);}

	vec3 w3 = w1+w2;
	float wf1 = max(max(w3.r,w3.g),w3.b);
	if (wf1 > 1.0) {wf1 = 1.0/wf1; w1*=wf1, w2*=wf1;}

	if (abs(clips) > 0.005)
	{
		sy = mc1; one = (clips > 0.0) ? w1 : 1.0.xxx;
		float sat = 1.0001-min(min(cref1.r,cref1.g),cref1.b);
		color1 = mix(color1, plant(pow(color1, 0.70.xxx-0.325*sat),sy), pow(sat,0.3333)*one*abs(clips));
		sy = mc2; one = (clips > 0.0) ? w2 : 1.0.xxx;
		sat =       1.0001-min(min(cref2.r,cref2.g),cref2.b);
		color2 = mix(color2, plant(pow(color2, 0.70.xxx-0.325*sat),sy), pow(sat,0.3333)*one*abs(clips));
	}
	
	color = (gc(color1)*w1 + gc(color2)*w2);
	
	color = min(color, 1.0);
}
	
	if (interb)
	{
		color = gc(color1);
	}

	float colmx = max(max(ctmp.r,ctmp.g),ctmp.b);

	if(!interb) color = pow( color, vec3(gamma_in/scangamma) );
  
	FragColor = vec4(color, colmx);
}

#version 450

/*
   CRT - Guest - Advanced - Fastest - Pass2
   
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
	float brightboost, brightboost1, gsl, scanline1, scanline2, beam_min, beam_max, beam_size,
    shadowMask, masksize, vertmask, slotmask, slotmask1, slotwidth, double_slot, mcut, maskDark, maskLight,
	maskstr, inters, bloom, halation, slotms, gamma_c, gamma_out, DER, DEG, DEB, DES, IOS, clips;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float warpX;
	float warpY;
	float csize;
	float bsize;
	float c_shape;
	float scan_falloff;
	float bloom_dist;
	float mshift;
	float mask_layout;
	float maskboost;
	float no_scanlines;
	float intres;
	float smask_mit;
	float mask_bloom;
	float mask_zoom;
	float mzoom_sh;
	float hmask1;
	float bloom_size;
	float scans;
	float post_br;
} global;


#pragma parameter bogus_screen "[ SCREEN OPTIONS ]: " 0.0 0.0 1.0 1.0

#pragma parameter intres "          Internal Resolution Y: 0.5...y-dowsample" 0.0 0.0 3.0 0.5 // Joint parameter with linearize pass, values must match

#pragma parameter IOS "          Integer Scaling: Odd:Y, Even:'X'+Y" 0.0 0.0 4.0 1.0
#define IOS          params.IOS     // Smart Integer Scaling

#pragma parameter warpX "          CurvatureX (default 0.03)" 0.0 0.0 0.25 0.01
#define warpX        global.warpX     // Curvature X

#pragma parameter warpY "          CurvatureY (default 0.04)" 0.0 0.0 0.25 0.01
#define warpY        global.warpY     // Curvature Y

#pragma parameter c_shape "          Curvature Shape" 0.25 0.05 0.60 0.05
#define c_shape        global.c_shape     // curvature shape

#pragma parameter csize "          Corner Size" 0.0 0.0 0.25 0.01
#define csize        global.csize     // corner size

#pragma parameter bsize "          Border smoothness" 400.0 100.0 700.0 10.0
#define bsize        global.bsize     // border smoothness 

#pragma parameter bogus_brightness "[ BRIGHTNESS SETTINGS ]:" 0.0 0.0 1.0 1.0

#pragma parameter bloom_size "          Bloom/Halation Size" 0.75 0.0 1.0 0.05
#define bloom_size         global.bloom_size     // bloom effect size

#pragma parameter bloom "          Bloom Strength" 0.0 0.0 2.0 0.05
#define bloom         params.bloom     // bloom effect

#pragma parameter mask_bloom "          Mask Bloom" 0.0 -2.0 2.0 0.05
#define mask_bloom         global.mask_bloom     // mask bloom effect

#pragma parameter bloom_dist "          Bloom Distribution" 0.0 0.0 3.0 0.05
#define bloom_dist         global.bloom_dist     // bloom effect distribution

#pragma parameter halation "          Halation Strength" 0.0 0.0 2.0 0.05
#define halation         params.halation     // halation effect

#pragma parameter hmask1 "          Halation Mask Strength" 0.5 -1.0 1.0 0.025
#define hmask1         global.hmask1     // bloom/halation mask strength

#pragma parameter gamma_c "          Gamma correct" 1.0 0.50 2.0 0.025
#define gamma_c   params.gamma_c     // adjust brightness

#pragma parameter brightboost "          Bright Boost Dark Pixels" 1.40 0.25 10.0 0.05
#define brightboost  params.brightboost     // adjust brightness

#pragma parameter brightboost1 "          Bright Boost Bright Pixels" 1.10 0.25 3.00 0.025
#define brightboost1  params.brightboost1     // adjust brightness

#pragma parameter clips "          Clip Saturated Color Beams" 0.0 -1.0 1.0 0.05
#define clips  params.clips   // works only in scanline mode

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

#pragma parameter beam_size "          Increased Bright Scanline Beam" 0.60 0.0 1.0 0.05
#define beam_size    params.beam_size     // increased max. beam size

#pragma parameter scans "          Scanline Saturation / Mask Falloff" 0.50 0.0 6.0 0.10
#define scans        1.5*global.scans     // scanline saturation

#pragma parameter scan_falloff "          Scanline Falloff" 1.0 0.10 2.0 0.05
#define scan_falloff        global.scan_falloff     // scanline falloff

#pragma parameter no_scanlines "          No-scanline mode" 0.0 0.0 1.5 0.05
#define no_scanlines global.no_scanlines

#pragma parameter bogus_masks "[ CRT MASK OPTIONS ]: " 0.0 0.0 1.0 1.0

#pragma parameter shadowMask "          CRT Mask: 0:CGWG, 1-4:Lottes, 5-13:'Trinitron'" 0.0 -1.0 13.0 1.0
#define shadowMask   params.shadowMask     // Mask Style

#pragma parameter maskstr "          Mask Strength (0, 5-12)" 0.3 -0.5 1.0 0.025
#define maskstr         params.maskstr      // Mask Strength

#pragma parameter mcut "          Mask 5-12 Low Strength" 1.10 0.0 2.0 0.05
#define mcut         params.mcut      // Mask 5-12 dark color strength

#pragma parameter maskboost "          CRT Mask Boost" 1.0 1.0 3.0 0.05
#define maskboost     global.maskboost     // Mask Boost

#pragma parameter masksize "          CRT Mask Size" 1.0 1.0 4.0 1.0
#define masksize     params.masksize     // Mask Size

#pragma parameter mask_zoom "          CRT Mask Zoom (+ mask width)" 0.0 -5.0 5.0 1.0
#define mask_zoom     global.mask_zoom     // Mask Size

#pragma parameter mzoom_sh "          CRT Mask Zoom Sharpen (needs Mask Zoom)" 0.0 0.0 1.0 0.05
#define mzoom_sh     global.mzoom_sh  

#pragma parameter maskDark "          Lottes maskDark" 0.5 0.0 2.0 0.05
#define maskDark     params.maskDark     // Dark "Phosphor"

#pragma parameter maskLight "          Lottes maskLight" 1.5 0.0 2.0 0.05
#define maskLight    params.maskLight     // Light "Phosphor"

#pragma parameter mshift "          (Transform to) Shadow Mask" 0.0 0.0 1.0 0.5
#define mshift     global.mshift     // do the "shadow mask"

#pragma parameter mask_layout "          Mask Layout: RGB or BGR (check LCD panel) " 0.0 0.0 1.0 1.0
#define mask_layout     global.mask_layout     // mask layout: RGB or BGR

#pragma parameter slotmask "          Slot Mask Strength Bright Pixels" 0.0 0.0 1.0 0.05
#define slotmask     params.slotmask

#pragma parameter slotmask1 "          Slot Mask Strength Dark Pixels" 0.0 0.0 1.0 0.05
#define slotmask1     params.slotmask1

#pragma parameter slotwidth "          Slot Mask Width (0:Auto)" 0.0 0.0 16.0 1.0
#define slotwidth    params.slotwidth     // Slot Mask Width

#pragma parameter double_slot "          Slot Mask Height: 2x1 or 4x1..." 2.0 1.0 4.0 1.0
#define double_slot  params.double_slot     // Slot Mask Height

#pragma parameter slotms "          Slot Mask Thickness" 1.0 1.0 4.0 1.0
#define slotms  params.slotms     // Slot Mask Thickness

#pragma parameter smask_mit "          Mitigate Slotmask Interaction" 0.0 0.0 1.0 0.05
#define smask_mit  global.smask_mit

#pragma parameter bogus_deconvergence22 "[ HORIZONTAL DECONVERGENCE ]:" 0.0 0.0 1.0 1.0

#pragma parameter vertmask "          Scanline Color Deconvergence" 0.0 -1.0 1.0 0.1
#define vertmask     params.vertmask     // Scanline deconvergence colors

#pragma parameter DER  "          Deconvergence Red offset" 0.0 -15.0 15.0 0.05

#pragma parameter DEG  "          Deconvergence Green offset" 0.0 -15.0 15.0 0.05

#pragma parameter DEB  "          Deconvergence Blue offset" 0.0 -15.0 15.0 0.05

#pragma parameter DES  "          Deconvergence Strength" 0.7 0.0 1.0 0.05

#pragma parameter post_br "          Post Brightness" 1.0 0.25 5.0 0.01

#pragma parameter gamma_out "          Gamma out" 2.4 1.0 5.0 0.05
#define gamma_out    params.gamma_out     // output gamma

#pragma parameter inters "          Interlacing Effect Smoothness" 0.0 0.0 0.5 0.05 // Joint parameter with linearize pass, values must match
#define inters  params.inters     // interlacing effect smoothing

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
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D LinearizePass;
layout(set = 0, binding = 4) uniform sampler2D PerfPass;
layout(set = 0, binding = 5) uniform sampler2D Pass2Feedback;

#define eps 1e-10 

float st(float x)
{
	return exp2(-10.0*x*x);
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

// Shadow mask (1-4 from PD CRT Lottes shader).

vec3 Mask(vec2 pos, float mx)
{
	vec3 mask = vec3(maskDark, maskDark, maskDark);
	vec3 one = vec3(1.0);
	
	if (shadowMask == 0.0)
	{
		float mc = 1.0 - max(maskstr, 0.0);	
		pos.x = fract(pos.x*0.5);
		if (pos.x < 0.49) { mask.r = 1.0; mask.g = mc; mask.b = 1.0; }
		else { mask.r = mc; mask.g = 1.0; mask.b = mc; }
	}    
   
	// Very compressed TV style shadow mask.
	else if (shadowMask == 1.0)
	{
		float line = maskLight;
		float odd  = 0.0;

		if (fract(pos.x/6.0) < 0.49)
			odd = 1.0;
		if (fract((pos.y + odd)/2.0) < 0.49)
			line = maskDark;

		pos.x = floor(mod(pos.x,3.0));
    
		if      (pos.x < 0.5) mask.r = maskLight;
		else if (pos.x < 1.5) mask.g = maskLight;
		else                  mask.b = maskLight;
		
		mask*=line;
	} 

	// Aperture-grille.
	else if (shadowMask == 2.0)
	{
		pos.x = floor(mod(pos.x,3.0));

		if      (pos.x < 0.5) mask.r = maskLight;
		else if (pos.x < 1.5) mask.g = maskLight;
		else                  mask.b = maskLight;
	} 

	// Stretched VGA style shadow mask (same as prior shaders).
	else if (shadowMask == 3.0)
	{
		pos.x += pos.y*3.0;
		pos.x  = fract(pos.x/6.0);

		if      (pos.x < 0.3) mask.r = maskLight;
		else if (pos.x < 0.6) mask.g = maskLight;
		else                  mask.b = maskLight;
	}

	// VGA style shadow mask.
	else if (shadowMask == 4.0)
	{
		pos.xy = floor(pos.xy*vec2(1.0, 0.5));
		pos.x += pos.y*3.0;
		pos.x  = fract(pos.x/6.0);

		if      (pos.x < 0.3) mask.r = maskLight;
		else if (pos.x < 0.6) mask.g = maskLight;
		else                  mask.b = maskLight;
	}
	
	// Trinitron mask 5
	else if (shadowMask == 5.0)
	{
		mask = vec3(0.0);		
		pos.x = fract(pos.x/2.0);
		if  (pos.x < 0.49)
		{	mask.r  = 1.0;
			mask.b  = 1.0;
		}
		else     mask.g = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);
	}    

	// Trinitron mask 6
	else if (shadowMask == 6.0)
	{
		mask = vec3(0.0);
		pos.x = floor(mod(pos.x,3.0));
		if      (pos.x < 0.5) mask.r = 1.0;
		else if (pos.x < 1.5) mask.g = 1.0;
		else                    mask.b = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);
	}
	
	// BW Trinitron mask 7
	else if (shadowMask == 7.0)
	{
		mask = vec3(0.0);		
		pos.x = fract(pos.x/2.0);
		if  (pos.x < 0.49)
		{	mask  = 0.0.xxx;
		}
		else     mask = 1.0.xxx;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);
	}    

	// BW Trinitron mask 8
	else if (shadowMask == 8.0)
	{
		mask = vec3(0.0);
		pos.x = fract(pos.x/3.0);
		if      (pos.x < 0.3) mask = 0.0.xxx;
		else if (pos.x < 0.6) mask = 1.0.xxx;
		else                  mask = 1.0.xxx;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);
	}    

	// Magenta - Green - Black mask
	else if (shadowMask == 9.0)
	{
		mask = vec3(0.0);
		pos.x = fract(pos.x/3.0);
		if      (pos.x < 0.3) mask    = 0.0.xxx;
		else if (pos.x < 0.6) mask.rb = 1.0.xx;
		else                  mask.g  = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);	
	}  
	
	// RGBX
	else if (shadowMask == 10.0)
	{
		mask = vec3(0.0);
		pos.x = fract(pos.x * 0.25);
		if      (pos.x < 0.2)  mask  = 0.0.xxx;
		else if (pos.x < 0.4)  mask.r = 1.0;
		else if (pos.x < 0.7)  mask.g = 1.0;	
		else                   mask.b = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);
	}  

	// 4k mask
	else if (shadowMask == 11.0)
	{
		mask = vec3(0.0);
		pos.x = fract(pos.x * 0.25);
		if      (pos.x < 0.2)  mask.r  = 1.0;
		else if (pos.x < 0.4)  mask.rg = 1.0.xx;
		else if (pos.x < 0.7)  mask.gb = 1.0.xx;	
		else                   mask.b  = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);
	}     

	// RRGGBBX mask
	else if (shadowMask == 12.0)
	{
		mask = vec3(0.0);
		pos.x = floor(mod(pos.x,7.0));
		if      (pos.x < 0.5)  mask   = 0.0.xxx;
		else if (pos.x < 2.5)  mask.r = 1.0;
		else if (pos.x < 4.5)  mask.g = 1.0;	
		else                   mask.b = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);
	}

	// 4k mask
	else
	{
		mask = vec3(0.0);
		pos.x = floor(mod(pos.x,6.0));
		if      (pos.x < 0.5)  mask   = 0.0.xxx;
		else if (pos.x < 1.5)  mask.r = 1.0;
		else if (pos.x < 2.5)  mask.rg = 1.0.xx;
		else if (pos.x < 3.5)  mask.rgb = 1.0.xxx;
		else if (pos.x < 4.5)  mask.gb = 1.0.xx;		
		else                   mask.b = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0);
	}
	
	if (mask_layout > 0.5) mask = mask.rbg;
	float maskmin = min(min(mask.r,mask.g),mask.b);
	return (mask - maskmin) * (1.0 + (maskboost-1.0)) + maskmin;
}


float SlotMask(vec2 pos, float m, float swidth)
{
	if ((slotmask + slotmask1) == 0.0) return 1.0;
	else
	{
	pos.y = floor(pos.y/slotms);
	float mlen = swidth*2.0;
	float px = floor(mod(pos.x, 0.99999*mlen));
	float py = floor(fract(pos.y/(2.0*double_slot))*2.0*double_slot);
	float slot_dark = mix(1.0-slotmask1, 1.0-slotmask, m);
	float slot = 1.0;
	if (py == 0.0 && px < swidth) slot = slot_dark; else
	if (py == double_slot && px >= swidth) slot = slot_dark;		
	
	return slot;
	}
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

vec2 Overscan(vec2 pos, float dx, float dy){
	pos=pos*2.0-1.0;    
	pos*=vec2(dx,dy);
	return pos*0.5+0.5;
}  

vec2 Warp(vec2 pos)
{
	pos  = pos*2.0-1.0;    
	pos  = mix(pos, vec2(pos.x*inversesqrt(1.0-c_shape*pos.y*pos.y), pos.y*inversesqrt(1.0-c_shape*pos.x*pos.x)), vec2(warpX, warpY)/c_shape);
	return pos*0.5 + 0.5;
}

// Borrowed from cgwg's crt-geom, under GPL

float corner(vec2 coord)
{
	coord = min(coord, vec2(1.0)-coord) * vec2(1.0, OutputSize.y/OutputSize.x);
	vec2 cdist = vec2(max(csize/3.0, max((1.0-smoothstep(100.0,600.0,bsize))*0.01,0.002)));
	coord = (cdist - min(coord,cdist));
	float dist = sqrt(dot(coord,coord));
	return clamp((cdist.x-dist)*bsize,0.0, 1.0);
}

vec3 calculate_bloom (vec2 pos, vec2 x, vec2 y)
{
	return  ( texture(LinearizePass, pos -x -y).rgb       +
	          texture(LinearizePass, pos    -y).rgb * 2.0 +
	          texture(LinearizePass, pos +x -y).rgb       +
	          texture(LinearizePass, pos -x   ).rgb * 2.0 +
	          texture(LinearizePass, pos      ).rgb * 4.0 +
	          texture(LinearizePass, pos +x   ).rgb * 2.0 +
	          texture(LinearizePass, pos -x +y).rgb       +
	          texture(LinearizePass, pos    +y).rgb * 2.0 +
	          texture(LinearizePass, pos +x +y).rgb        )*0.0625;				   			   
}

void main()
{
	float intera = texture(LinearizePass, vec2(0.75,0.25)).a;
	bool interb  = ((intera < 0.75) || (no_scanlines > 0.025));

	vec4 result = texture(Pass2Feedback, vTexCoord);
	
	vec2 texcoord = vTexCoord * 1.00001;

	if (IOS > 0.0 && !interb){
		vec2 ofactor = OutputSize.xy/global.OriginalSize.xy;
		vec2 intfactor = (IOS < 2.5) ? floor(ofactor) : ceil(ofactor);
		vec2 diff = ofactor/intfactor;
		float scan = diff.y;
		texcoord = Overscan(texcoord, scan, scan);
		if (IOS == 1.0 || IOS == 3.0) texcoord = vec2(vTexCoord.x, texcoord.y);
	}
	
	vec2 yy = vec2(0.0, global.OriginalSize.w);
	vec2 xx = vec2(global.OriginalSize.z, 0.0);
	vec2 y2 = yy+yy;
	vec2 pos  = Warp(texcoord);	

	bool not_same = (texture(PerfPass, pos   ).a < 0.5);
	vec3 color = 0.0.xxx;

if ( (not_same || (interb && (no_scanlines < 0.025))) )
{
	vec4 SourceSize = global.OriginalSize;
	float SourceY = SourceSize.y;
	float sy = 1.0;
	if (global.intres > 0.25) sy = global.intres;
	SourceSize*=vec4(1.0, 1.0/sy, 1.0, sy);
	
	float gamma_in = 1.0/texture(LinearizePass, vec2(0.25,0.25)).a;

	// Calculating texel coordinates

	vec2 cpos = (IOS > 2.5) ? vTexCoord : texcoord;
	float corner0 = corner(Warp(cpos));

	float coffset = 0.5;
	
	vec2 ps = SourceSize.zw;
	float OGL2Pos = pos.y * SourceSize.y - coffset;
	float f = fract(OGL2Pos);
	
	vec2 dx = vec2(ps.x,0.0);
	vec2 dy = vec2(0.0, ps.y);
   
	// Reading the texels
	
	vec2 pC4;
	pC4.y = floor(OGL2Pos) * ps.y + 0.5*ps.y; 
	pC4.x = pos.x;

	if (global.intres == 0.5) pC4.y = floor(pC4.y * global.OriginalSize.y)*global.OriginalSize.w + 0.5*global.OriginalSize.w;
	
	if (interb && no_scanlines < 0.025)  pC4.y = pos.y - inters * SourceSize.w; else if (interb) pC4.y = pC4.y + smoothstep(0.40-0.5*no_scanlines, 0.60 + 0.5*no_scanlines, f)*SourceSize.w;
	
	vec3 color1 = texture(Source, pC4      ).rgb;
	vec3 dcolor1 = color1;
	dcolor1.r = texture(Source, pC4 + dx*params.DER).r;
	dcolor1.g = texture(Source, pC4 + dx*params.DEG).g;
	dcolor1.b = texture(Source, pC4 + dx*params.DEB).b;
	color1 = mix(color1, dcolor1, params.DES);
	
	vec3 scolor1 = 	texture(Source, pC4      ).aaa;
	
	if (interb && no_scanlines < 0.025) pC4.y = pos.y + inters * SourceSize.w; else
	pC4+=dy;

	if (global.intres == 0.5) pC4.y = floor((pos.y + 0.33*dy.y) * global.OriginalSize.y)*global.OriginalSize.w + 0.5*global.OriginalSize.w;
	
	vec3 color2 = texture(Source, pC4      ).rgb;
	vec3 dcolor2 = color2;
	dcolor2.r = texture(Source, pC4 + dx*params.DER).r;
	dcolor2.g = texture(Source, pC4 + dx*params.DEG).g;
	dcolor2.b = texture(Source, pC4 + dx*params.DEB).b;
	color2 = mix(color2, dcolor2, params.DES);

	vec3 scolor2 = 	texture(Source, pC4      ).aaa;
	
	// calculating scanlines

	vec3 ctmp = color1; vec3 mcolor = scolor1; float w3 = 1.0; color = color1;
	vec3 one = vec3(1.0);
	vec3 clip = one;	
	
if (!interb)
{	
	float shape1 = mix(scanline1, scanline2, f);
	float shape2 = mix(scanline1, scanline2, 1.0-f);
	
	float wt1 = st(f);
	float wt2 = st(1.0-f);

	vec3 color00 = color1*wt1 + color2*wt2;
	vec3 scolor0 = scolor1*wt1 + scolor2*wt2;
	
	ctmp = color00/(wt1+wt2);
	vec3 sctmp = max(scolor0/(wt1+wt2), ctmp);
	mcolor = sctmp;
	
	vec3 w1, w2;
	
	vec3 cref1 = mix(sctmp, scolor1, beam_size); float creff1 = pow(max(max(cref1.r,cref1.g),cref1.b), scan_falloff);
	vec3 cref2 = mix(sctmp, scolor2, beam_size); float creff2 = pow(max(max(cref2.r,cref2.g),cref2.b), scan_falloff);

	float f1 = f; 
	float f2 = 1.0-f;

	float mc1 = max(max(color1.r,color1.g),color1.b) + eps;
	float mc2 = max(max(color2.r,color2.g),color2.b) + eps;
	
	cref1 = color1 / mc1;
	cref2 = color2 / mc2;

	if (gsl <  0.5) { w1 = sw0(f1,creff1,shape1,cref1); w2 = sw0(f2,creff2,shape2,cref2);} else
	if (gsl == 1.0) { w1 = sw1(f1,creff1,shape1,cref1); w2 = sw1(f2,creff2,shape2,cref2);} else
	                { w1 = sw2(f1,creff1,shape1,cref1); w2 = sw2(f2,creff2,shape2,cref2);}

	vec3 w4 = w1+w2;
	float wf1 = max(max(w4.r,w4.g),w4.b); w3 = wf1;
	if (wf1 > 1.0) {wf1 = 1.0/wf1; w1*=wf1, w2*=wf1;}
	
	// Scanline Deconvergence
	vec3 cd1 = one; vec3 cd2 = one; 
	
if (abs(vertmask) > 0.025)
{
	float vm = sqrt(abs(vertmask));
	float v_high1 = 1.0 + 0.3*vm;
	float v_high2 = 1.0 + 0.6*vm;	
	float v_low  = 1.0 - vm;
	
	float ds1 = min(max(1.0-w3*w3, 2.5*f1), 1.0);
	float ds2 = min(max(1.0-w3*w3, 2.5*f2), 1.0);
	
	if (vertmask < 0.025) 
	{
		cd1 = mix(one, vec3(v_high2, v_low, v_low), ds1);
		cd2 = mix(one, vec3(v_low, v_high1, v_high1), ds2);
	}
	else
	{
		cd1 = mix(one, vec3(v_high1, v_low, v_high1), ds1);
		cd2 = mix(one, vec3(v_low, v_high2, v_low), ds2);
	}
}
	float aclips = abs(clips);
	if (aclips > 0.005)
	{
		sy = mc1; clip = (clips > 0.0) ? w1 : 1.0.xxx;
		float sat = 1.0001-min(min(cref1.r,cref1.g),cref1.b);
		color1 = mix(color1, plant(pow(color1, 0.70.xxx-0.325*sat),sy), pow(sat,0.3333)*clip*aclips);
		sy = mc2; clip = (clips > 0.0) ? w2 : 1.0.xxx;
		sat =       1.0001-min(min(cref2.r,cref2.g),cref2.b);
		color2 = mix(color2, plant(pow(color2, 0.70.xxx-0.325*sat),sy), pow(sat,0.3333)*clip*aclips);
	}	
	
	color = color1*w1*cd1 + color2*w2*cd2;
	color = min(color, 1.0);
}	
	if (interb) 
	{
		color = color1;
		mcolor = scolor1;
		ctmp = color;
	}

	color = gc(color);

	float mx = max(max(mcolor.r,mcolor.g),mcolor.b);
	float mxg = mx;
	mx = pow(mx, 1.40/gamma_in);

	vec3 Bloom = calculate_bloom (pos, xx*bloom_size, 0.75*yy*bloom_size);

	float colmx = max(max(ctmp.r, ctmp.g), ctmp.b);
   
	// Apply Mask
	
	vec3 orig1 = color;
	vec3 cmask = one;

	float mask_compensate = 1.0;

if (shadowMask > -0.5)
{	
	// mask widths and mask dark compensate (fractional part) values
	float mwidths[14] = float[14] (2.0, 3.0, 3.0, 6.0, 6.0, 2.0, 3.0, 2.0, 3.0, 3.0, 4.0, 4.0, 7.0, 6.0); 
	
	float mwidth = mwidths[int(shadowMask)];
	if (shadowMask > 4.5) mask_compensate = 1.325;
	vec2 maskcoord = gl_FragCoord.xy * 1.00001;
	vec2 scoord = maskcoord;
	
	mwidth = mwidth * masksize;
	bool zoomed = (abs(mask_zoom) > 0.5);
	float mscale = 1.0;
	vec2 maskcoord0 = maskcoord;
	maskcoord.y = floor(maskcoord.y/masksize);
	float mwidth1 = max(mwidth + mask_zoom, 2.0);
	float swidth = mwidth1;

if ( mshift > 0.25 )
{	
	float stagg_lvl = 1.0; if (fract(mshift) > 0.25) stagg_lvl = 2.0;
	float next_line = float(floor(mod(maskcoord.y, 2.0*stagg_lvl)) < stagg_lvl);
	maskcoord0.x = maskcoord0.x + next_line * floor(0.5 * mwidth1);
}		
	maskcoord = maskcoord0/masksize;

if ( !zoomed )	
	cmask*= Mask(floor(maskcoord), mx);
else{
	mscale  = mwidth1/mwidth;
	float mlerp = fract(maskcoord.x/mscale); if (mzoom_sh > 0.025) mlerp = clamp((1.0 + mzoom_sh)*mlerp - 0.5*mzoom_sh, 0.0, 1.0);
	float mcoord = floor(maskcoord.x/mscale); if (shadowMask == 12.0 && mask_zoom == -2.0) mcoord = ceil(maskcoord.x/mscale);
	cmask*=mix(Mask(vec2(mcoord,maskcoord.y), mx), Mask(vec2(mcoord + 1.0, maskcoord.y), mx), mlerp);
}

	if (slotwidth > 0.5) swidth = slotwidth; float smask = 1.0;

	float sm_offset = 0.0; bool bsm_offset = (shadowMask == 0.0 || shadowMask == 2.0 || shadowMask == 5.0 || shadowMask == 6.0 || shadowMask == 8.0 || shadowMask == 11.0);
	if( zoomed ) { if (mask_layout < 0.5 && bsm_offset) sm_offset = 1.0; else if (bsm_offset) sm_offset = -1.0; }
	
	smask = SlotMask(scoord + vec2(sm_offset,0.0), mx, swidth);

	smask = clamp(smask + mix(smask_mit, 0.0, min(w3, pow(w3*max(max(orig1.r,orig1.g),orig1.b), 0.33333))), 0.0, 1.0);
	
	cmask*=smask;

	if (abs(mask_bloom) > 0.025)
	{
		float maxbl = max(max(max(Bloom.r,Bloom.g),Bloom.b), mxg);
		maxbl = maxbl * max(mix(1.0, 2.0-colmx, bloom_dist), 0.0);
		if (mask_bloom > 0.025) cmask = max(min(cmask + maxbl*mask_bloom, 1.0), cmask); else cmask = max(mix(cmask, cmask*(1.0-0.5*maxbl) + plant(pow(Bloom,0.35.xxx),maxbl), -mask_bloom),cmask);
	}
	
	color = color*cmask;
	color = min(color,1.0);
	cmask  = min(cmask, 1.0);
}
	float bb = mix(brightboost*mask_compensate, brightboost1, mx);
	color*=bb;

if(bloom > 0.025)
{
	vec3 Bloom1 = min(Bloom*(orig1+color), max(0.5*(colmx + orig1 - color),0.0));
	Bloom1 = 0.5*(Bloom1 + mix(Bloom1, mix(colmx*orig1, Bloom1, 0.5), 1.0-color));
	Bloom1 = Bloom1 * mix(1.0, 2.0-colmx, bloom_dist);
	color = color + bloom * Bloom1;
}

	vec3 hcmask = mix(one, cmask, hmask1);

if (halation > 0.025)
{
	float mbl = max(max(Bloom.r,Bloom.g),Bloom.b);
	Bloom = plant(Bloom + orig1 + Bloom*Bloom*Bloom, min(mbl*mbl,0.75));
	color = color + 2.0*mix(1.0,w3,0.5*colmx)*hcmask*Bloom* mix(1.0, 2.0-colmx, bloom_dist)*halation; 
}
	
	color = clamp(color, 0.0, 1.0);
	color = pow(color, vec3(1.0/gamma_out));
	
	result.rgb = color*corner0*global.post_br;

}
	FragColor = vec4(result.rgb, 1.0);
} 
#version 450

/*
   CRT - Guest - Advanced - Fastest - Pass2
   
   Copyright (C) 2018-2022 guest(r) - guest.r@gmail.com

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
    glow, shadowMask, masksize, vertmask, slotmask, slotwidth, double_slot, mcut, maskDark, maskLight,
	maskstr, inters, bloom, halation, scans, slotms, mclip, gamma_c, gamma_out, DER, DEG, DEB, DES, IOS;
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
	float slotmask1;
	float scan_falloff;
	float bloom_dist;
	float mshift;
	float mask_layout;
	float maskboost;
	float no_scanlines;
	float intres;
	float fcompat;
	float smask_mit;
	float ssharp;
} global;

#pragma parameter fcompat "          Compatibility Mode (slower, use if glitchy)" 0.0 0.0 1.0 1.0 

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

#pragma parameter glow "          Glow Strength" 0.08 0.0 2.0 0.01
#define glow         params.glow     // Glow Strength

#pragma parameter bloom "          Bloom Strength" 0.0 0.0 2.0 0.05
#define bloom         params.bloom     // bloom effect

#pragma parameter bloom_dist "          Bloom Distribution" 0.0 0.0 3.0 0.05
#define bloom_dist         global.bloom_dist     // bloom effect distribution

#pragma parameter halation "          Halation Strength" 0.0 0.0 1.0 0.025
#define halation         params.halation     // halation effect

#pragma parameter gamma_c "          Gamma correct" 1.0 0.50 2.0 0.025
#define gamma_c   params.gamma_c     // adjust brightness

#pragma parameter brightboost "          Bright Boost Dark Pixels" 1.40 0.25 10.0 0.05
#define brightboost  params.brightboost     // adjust brightness

#pragma parameter brightboost1 "          Bright Boost Bright Pixels" 1.10 0.25 3.00 0.025
#define brightboost1  params.brightboost1     // adjust brightness

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

#pragma parameter vertmask "          Scanline Color Deconvergence" 0.0 -1.0 1.0 0.1
#define vertmask     params.vertmask     // Scanline deconvergence colors

#pragma parameter scans "          Scanline Saturation / Mask Falloff" 0.50 -5.0 5.0 0.10
#define scans        params.scans     // scanline saturation

#pragma parameter scan_falloff "          Scanline Falloff" 1.0 0.10 2.0 0.05
#define scan_falloff        global.scan_falloff     // scanline falloff

#pragma parameter ssharp "          Smart Sharpen Scanlines" 0.0 0.0 0.30 0.01
#define ssharp global.ssharp

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

#pragma parameter maskDark "          Lottes maskDark" 0.5 0.0 2.0 0.05
#define maskDark     params.maskDark     // Dark "Phosphor"

#pragma parameter maskLight "          Lottes maskLight" 1.5 0.0 2.0 0.05
#define maskLight    params.maskLight     // Light "Phosphor"

#pragma parameter mshift "          Mask Shift/Stagger" 0.0 -8.0 8.0 0.5
#define mshift     global.mshift     // mask 'line' shift/stagger

#pragma parameter mask_layout "          Mask Layout: RGB or BGR (check LCD panel) " 0.0 0.0 1.0 1.0
#define mask_layout     global.mask_layout     // mask layout: RGB or BGR

#pragma parameter slotmask "          Slot Mask Strength Bright Pixels" 0.0 0.0 1.0 0.05
#define slotmask     params.slotmask

#pragma parameter slotmask1 "          Slot Mask Strength Dark Pixels" 0.0 0.0 1.0 0.05
#define slotmask1     global.slotmask1

#pragma parameter slotwidth "          Slot Mask Width (0:Auto)" 0.0 0.0 16.0 1.0
#define slotwidth    params.slotwidth     // Slot Mask Width

#pragma parameter double_slot "          Slot Mask Height: 2x1 or 4x1..." 2.0 1.0 4.0 1.0
#define double_slot  params.double_slot     // Slot Mask Height

#pragma parameter slotms "          Slot Mask Thickness" 1.0 1.0 4.0 1.0
#define slotms  params.slotms     // Slot Mask Thickness

#pragma parameter mclip "          Keep Mask effect with clipping" 0.0 0.0 1.0 0.05
#define mclip  params.mclip     // Slot Mask Size

#pragma parameter smask_mit "          Mitigate Slotmask Interaction" 0.0 0.0 1.0 0.05
#define smask_mit  global.smask_mit

#pragma parameter bogus_deconvergence22 "[ HORIZONTAL DECONVERGENCE ]:" 0.0 0.0 1.0 1.0

#pragma parameter DER  "          Deconvergence Red offset" 0.0 -15.0 15.0 0.5

#pragma parameter DEG  "          Deconvergence Green offset" 0.0 -15.0 15.0 0.5

#pragma parameter DEB  "          Deconvergence Blue offset" 0.0 -15.0 15.0 0.5

#pragma parameter DES  "          Deconvergence Strength" 0.7 0.0 1.0 0.05

#pragma parameter gamma_out "          Gamma out" 2.4 1.0 5.0 0.05
#define gamma_out    params.gamma_out     // output gamma

#pragma parameter inters "          Interlacing Effect Smoothness" 0.0 0.0 0.5 0.05 // Joint parameter with linearize pass, values must match
#define inters  params.inters     // interlacing effect smoothing


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
   vTexCoord = TexCoord * 1.00000;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D LinearizePass;
layout(set = 0, binding = 4) uniform sampler2D PrePassDontChange;
layout(set = 0, binding = 5) uniform sampler2D Pass2Feedback;

#define eps 1e-10 

float st(float x)
{
	return exp2(-10.0*x*x);
} 
   
float sw0(float x, float color, float scanline)
{
	float tmp = mix(beam_min, beam_max, color);
	float ex = x*tmp;
	ex = (gsl > -0.5) ? ex*ex : mix(ex*ex, ex*ex*ex, 0.4);
	return exp2(-scanline*ex);
} 

float sw1(float x, float color, float scanline)
{	
	x = mix (x, beam_min*x, max(x-0.4*color,0.0));
	float tmp = mix(1.2*beam_min, beam_max, color);
	float ex = x*tmp;
	return exp2(-scanline*ex*ex);
}    

float sw2(float x, float color, float scanline)
{
	float tmp = mix((2.5-0.5*color)*beam_min, beam_max, color);
	tmp = mix(beam_max, tmp, pow(x, color+0.3));
	float ex = x*tmp;
	return exp2(-scanline*ex*ex);
}  

// Shadow mask (1-4 from PD CRT Lottes shader).

vec3 Mask(vec2 pos, float mx, inout float swidth)
{
	vec2 pos0 = pos;
	pos.y = floor(pos.y/masksize);
	float stagg_lvl = 1.0; if (fract(abs(mshift)) > 0.25 && abs(mshift) > 1.25) stagg_lvl = 2.0;
	float next_line = float(fract((pos.y/stagg_lvl)*0.5) > 0.25);
	pos0.x = (mshift > -0.25) ? (pos0.x + next_line * floor(mshift)) : (pos0.x + floor(pos.y / stagg_lvl) * floor(abs(mshift)));
	pos = floor(pos0/masksize);

	vec3 mask = vec3(maskDark, maskDark, maskDark);
	vec3 one = vec3(1.0);
	
	// brightness correcture masks 5-12
	float sm = 0.45; if(shadowMask == 8.0 || shadowMask == 11.0 || shadowMask == 13.0) sm = 0.7;
	float dark_compensate  = mix(max( clamp( mix (mcut, maskstr, mx),0.0, 1.0) - sm, 0.0) + 1.0, 1.0, mx);
	
	float mc = 1.0 - max(maskstr, 0.0);	
	
	// No mask
	if (shadowMask == -1.0)
	{
		mask = vec3(1.0);
	}       
	
	// Phosphor.
	else if (shadowMask == 0.0)
	{
		pos.x = fract(pos.x*0.5);
		if (pos.x < 0.49) { mask.r = 1.0; mask.g = mc; mask.b = 1.0; }
		else { mask.r = mc; mask.g = 1.0; mask.b = mc; }
		swidth = 2.0;
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
		swidth = 6.0;
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
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;
		swidth = 2.0;
	}    

	// Trinitron mask 6
	else if (shadowMask == 6.0)
	{
		mask = vec3(0.0);
		pos.x = floor(mod(pos.x,3.0));
		if      (pos.x < 0.5) mask.r = 1.0;
		else if (pos.x < 1.5) mask.g = 1.0;
		else                    mask.b = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;
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
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;
		swidth = 2.0;		
	}    

	// BW Trinitron mask 8
	else if (shadowMask == 8.0)
	{
		mask = vec3(0.0);
		pos.x = fract(pos.x/3.0);
		if      (pos.x < 0.3) mask = 0.0.xxx;
		else if (pos.x < 0.6) mask = 1.0.xxx;
		else                  mask = 1.0.xxx;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;
	}    

	// Magenta - Green - Black mask
	else if (shadowMask == 9.0)
	{
		mask = vec3(0.0);
		pos.x = fract(pos.x/3.0);
		if      (pos.x < 0.3) mask    = 0.0.xxx;
		else if (pos.x < 0.6) mask.rb = 1.0.xx;
		else                  mask.g  = 1.0;
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;	
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
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;
		swidth = 4.0;
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
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;
		swidth = 4.0;		
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
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;
		swidth = 7.0;
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
		mask = clamp(mix( mix(one, mask, mcut), mix(one, mask, maskstr), mx), 0.0, 1.0) * dark_compensate;
		swidth = 6.0;
	}
	
	swidth*=masksize;
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


vec3 declip(vec3 c, float b)
{
	float m = max(max(c.r,c.g),c.b);
	if (m > b) c = c*b/m;
	return c;
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
	return  ( COMPAT_TEXTURE(LinearizePass, pos -x -y).rgb * 0.091849 +
	          COMPAT_TEXTURE(LinearizePass, pos    -y).rgb * 0.119368 +
	          COMPAT_TEXTURE(LinearizePass, pos +x -y).rgb * 0.091849 +
	          COMPAT_TEXTURE(LinearizePass, pos -x   ).rgb * 0.119368 +
	          COMPAT_TEXTURE(LinearizePass, pos      ).rgb * 0.155131 +
	          COMPAT_TEXTURE(LinearizePass, pos +x   ).rgb * 0.119368 +
	          COMPAT_TEXTURE(LinearizePass, pos -x +y).rgb * 0.091849 +
	          COMPAT_TEXTURE(LinearizePass, pos    +y).rgb * 0.119368 +
	          COMPAT_TEXTURE(LinearizePass, pos +x +y).rgb * 0.091849 );				   			   
}

void main()
{
	float intera = COMPAT_TEXTURE(LinearizePass, vec2(0.75,0.25)).a;
	bool interb  = ((intera < 0.75) || (no_scanlines > 0.025));

	vec4 result = COMPAT_TEXTURE(Pass2Feedback, vTexCoord);

	vec2 texcoord = TEX0.xy;

	if (IOS > 0.0 && !interb){
		vec2 ofactor = OutputSize.xy/global.OriginalSize.xy;
		vec2 intfactor = (IOS < 2.5) ? floor(ofactor) : ceil(ofactor);
		vec2 diff = ofactor/intfactor;
		float scan = diff.y;
		texcoord = Overscan(texcoord, scan, scan);
		if (IOS == 1.0 || IOS == 3.0) texcoord = vec2(TEX0.x, texcoord.y);
	}
	
	vec2 yy = vec2(0.0, global.OriginalSize.w);
	vec2 xx = vec2(global.OriginalSize.z, 0.0);
	vec2 y2 = yy+yy;
	vec2 pos  = Warp(texcoord);	
	vec2 pc4 = pos;
	pc4 = floor(pc4 * global.OriginalSize.xy - vec2(0.0, 0.5)) * global.OriginalSize.zw + 0.5*global.OriginalSize.zw; 

	float same2 = COMPAT_TEXTURE(PrePassDontChange,pc4-yy).a;
	float same3 = COMPAT_TEXTURE(PrePassDontChange,pc4   ).a;
	float same4 = COMPAT_TEXTURE(PrePassDontChange,pc4+yy).a;
	float same5 = COMPAT_TEXTURE(PrePassDontChange,pc4+y2).a;
	
	float refresh1 = 60.0;
	float refresh2 = round(TEX0.y*59.0);
	
	bool frames = (abs(floor(mod(float(global.FrameCount), refresh1)) - refresh2) < 0.001);
	bool not_same = (same2 + same3 + same4 + same5) > 0.5;

if ( not_same || frames || (interb && (no_scanlines < 0.025)) || global.fcompat > 0.5)

{

	vec4 SourceSize = global.OriginalSize;
	float SourceY = SourceSize.y;
	float sy = 1.0;
	if (global.intres == 1.0) sy = SourceY/224.0;
	if (global.intres > 0.25 && global.intres != 1.0) sy = global.intres;
	SourceSize*=vec4(1.0, 1.0/sy, 1.0, sy);
	
	
	float gamma_in = 1.0/COMPAT_TEXTURE(LinearizePass, vec2(0.25,0.25)).a;

	// Calculating texel coordinates

	vec2 cpos = (IOS > 2.5) ? TEX0 : texcoord;
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
	
	vec3 color1 = COMPAT_TEXTURE(Source, pC4      ).rgb;
	vec3 dcolor1 = color1;
	dcolor1.r = COMPAT_TEXTURE(Source, pC4 + dx*params.DER).r;
	dcolor1.g = COMPAT_TEXTURE(Source, pC4 + dx*params.DEG).g;
	dcolor1.b = COMPAT_TEXTURE(Source, pC4 + dx*params.DEB).b;
	color1 = mix(color1, dcolor1, params.DES);
	
	vec3 scolor1 = 	COMPAT_TEXTURE(Source, pC4      ).aaa;
	
	if (interb && no_scanlines < 0.025) pC4.y = pos.y + inters * SourceSize.w; else
	pC4+=dy;

	if (global.intres == 0.5) pC4.y = floor((pos.y + 0.33*dy.y) * global.OriginalSize.y)*global.OriginalSize.w + 0.5*global.OriginalSize.w;
	
	vec3 color2 = COMPAT_TEXTURE(Source, pC4      ).rgb;
	vec3 dcolor2 = color2;
	dcolor2.r = COMPAT_TEXTURE(Source, pC4 + dx*params.DER).r;
	dcolor2.g = COMPAT_TEXTURE(Source, pC4 + dx*params.DEG).g;
	dcolor2.b = COMPAT_TEXTURE(Source, pC4 + dx*params.DEB).b;
	color2 = mix(color2, dcolor2, params.DES);

	vec3 scolor2 = 	COMPAT_TEXTURE(Source, pC4      ).aaa;
	
	// calculating scanlines

	vec3 ctmp = color1; vec3 mcolor = scolor1; float w3 = 1.0; vec3 color = color1;
	vec3 one = vec3(1.0);

if (!interb)
{	
	vec3 luma = vec3(0.2126, 0.7152, 0.0722);	
	float ssub = ssharp*max(abs(scolor1.x-scolor2.x), abs(dot(color1,luma)-dot(color2,luma)));

	float shape1 = mix(scanline1, scanline2 + ssub * scolor1.x * 35.0, f);
	float shape2 = mix(scanline1, scanline2 + ssub * scolor2.x * 35.0, 1.0-f);
	
	float wt1 = st(f);
	float wt2 = st(1.0-f);

	vec3 color00 = color1*wt1 + color2*wt2;
	vec3 scolor0 = scolor1*wt1 + scolor2*wt2;
	
	ctmp = color00/(wt1+wt2);
	vec3 sctmp = max(scolor0/(wt1+wt2), ctmp);
	mcolor = sctmp;
	
	float wf1, wf2;
	
	vec3 cref1 = mix(sctmp, scolor1, beam_size); float creff1 = pow(max(max(cref1.r,cref1.g),cref1.b), scan_falloff);
	vec3 cref2 = mix(sctmp, scolor2, beam_size); float creff2 = pow(max(max(cref2.r,cref2.g),cref2.b), scan_falloff);

	float f1 = f; 
	float f2 = 1.0-f;
	
	if (gsl <  0.5) { wf1 = sw0(f1,creff1,shape1); wf2 = sw0(f2,creff2,shape2);} else
	if (gsl == 1.0) { wf1 = sw1(f1,creff1,shape1); wf2 = sw1(f2,creff2,shape2);} else
	                { wf1 = sw2(f1,creff1,shape1); wf2 = sw2(f2,creff2,shape2);}

	if ((wf1 + wf2) > 1.0) { float wtmp = 1.0/(wf1+wf2); wf1*=wtmp; wf2*=wtmp; }
	
	// Scanline saturation application
	
	vec3 w1 = vec3(wf1); vec3 w2 = vec3(wf2);
	w3 = wf1+wf2;
	
	float mc1 = max(max(color1.r,color1.g),color1.b) + eps;
	float mc2 = max(max(color2.r,color2.g),color2.b) + eps;
	
	cref1 = color1 / mc1;
	cref2 = color2 / mc2;

	float scanpow1 = (scans > 0.0) ? 1.0 : pow(f1, 0.33);
	float scanpow2 = (scans > 0.0) ? 1.0 : pow(f2, 0.33);	
	w1 = pow(w1, mix(2.0*abs(scans).xxx + 1.0, 1.0.xxx, mix(1.0.xxx, cref1, scanpow1)));
	w2 = pow(w2, mix(2.0*abs(scans).xxx + 1.0, 1.0.xxx, mix(1.0.xxx, cref2, scanpow2)));
	
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
	
	if (vertmask < 0.0) 
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
	
	color = (gc(color1)*w1*cd1 + gc(color2)*w2*cd2);
	color = min(color, 1.0);
}
	
	if (interb) 
	{
		color = gc(color1);
		mcolor = scolor1;
	}

	float mx = max(max(mcolor.r,mcolor.g),mcolor.b);
	mx = pow(mx, 1.20/gamma_in);
   
	// Apply Mask
	
	vec3 orig1 = color;
	vec3 cmask = one;
	
	float swidth = 3.0;
	cmask*= Mask(gl_FragCoord.xy * 1.000001, mx, swidth);
	if (slotwidth > 0.5) swidth = slotwidth;
	float smask = SlotMask(gl_FragCoord.xy * 1.000001, mx, swidth);
	smask = clamp(smask + mix(smask_mit, 0.0, min(w3, pow(w3*max(max(orig1.r,orig1.g),orig1.b), 0.33333))), 0.0, 1.0);

	if (mask_layout > 0.5) cmask = cmask.rbg;
	
	color = color*cmask;
	color = min(color,1.0);
	color = color*smask;

	cmask = min(cmask*smask, 1.0);

	if (interb) ctmp = color;
	float colmx = pow( max( max(ctmp.r, ctmp.g), ctmp.b), 1.40/gamma_out);
	float bb = mix(brightboost, brightboost1, colmx);
	color*=bb;

	vec3  Glow = calculate_bloom (pos, xx, 0.75*yy);
	float maxb = max(max(Glow.r, Glow.g),Glow.b);
	Glow = pow(Glow, 1.4.xxx);	
	vec3  Bloom = Glow;

if(bloom > 0.025)
{
	vec3 Bloom1 = min(Bloom*(orig1+color), max(0.5*(colmx + orig1 - color),0.0));
	Bloom1 = 0.5*(Bloom1 + mix(Bloom1, mix(colmx*orig1, Bloom1, 0.5), 1.0-color));
	Bloom1 = Bloom1 * mix(1.0, 2.0-colmx, bloom_dist);
	color = color + bloom * Bloom1;
}

	if (!interb) color = declip(color, pow(w3,0.60));

	if (halation > 0.025) {
		Bloom = mix(0.5*(Bloom + Bloom*Bloom), 0.75*Bloom*Bloom, colmx);
		color = color + 0.75*(0.75+maxb)*Bloom*(0.4+sqrt(colmx))*mix(1.0,w3,0.5*colmx)*mix(one,cmask,0.35 + 0.4*maxb)*halation; }

	Glow = mix(Glow, 0.25*color, colmx);
	color = color + 0.5*glow*Glow;

	color = min(color, mix(one, cmask, mclip));
	
	color = pow(color, vec3(1.0/gamma_out));
	color = min(color, 1.0);
	
	result = vec4(color*corner0, corner0);

}
	FragColor = result;
} 
/*
   Interlacing
   
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

#include "../base/common/globals-and-screen-scale-params.inc"
#include "../base/common/common-functions.inc"

layout(push_constant) uniform Push
{
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	// float GAMMA_INPUT;
	// float inter;
	// float interm;
	// float iscan;
	// float intres;
	// float iscans;
	// float downsample_levelx;
	// float downsample_levely;
	// float gamma_out;
	// float vga_mode;
	// float hiscan;
} params;


/*
layout(std140, set = 0, binding = 0) uniform UBO
{
	vec4 SourceSize;
	mat4 MVP;
} global;
*/

// #pragma parameter bogus_gamma "[ GAMMA OPTIONS ]: " 0.0 0.0 0.0 1.0

// #pragma parameter GAMMA_INPUT "          Gamma Input" 2.4 1.0 5.0 0.05
// #define GAMMA_INPUT params.GAMMA_INPUT

// #pragma parameter gamma_out "          Gamma out" 2.4 1.0 5.0 0.05
// #define gamma_out    params.gamma_out     // output gamma

// #pragma parameter bogus_interlacing "[ INTERLACING OPTIONS ]: " 0.0 0.0 0.0 1.0

// #pragma parameter inter "          Interlace Trigger Resolution :" 375.0 0.0 800.0 25.0
// #define inter         params.inter     // interlace resolution

// #pragma parameter interm "          Interlace Mode: OFF, Normal 1-3, Interpolation 4-5" 1.0 0.0 5.0 1.0

#define interm      HSM_INTERLACE_MODE     // interlace mode 


// #pragma parameter inters "          Interlacing Effect Smoothness" 0.0 0.0 0.5 0.05 // Joint parameter with main pass, values must match

#define inters  	HSM_INTERLACE_EFFECT_SMOOTHNESS_INTERS     // interlacing effect smoothing


// #pragma parameter iscan "          Interlacing Scanline Effect" 0.20 0.0 1.0 0.05

#define iscan  		HSM_INTERLACE_SCANLINE_EFFECT     // interlacing effect scanlining


// #pragma parameter intres "          Internal Resolution Y: 224p/240p, 1.5...y-dowsample" 0.0 0.0 6.0 0.5 // Joint parameter with main pass, values must match

// #define intres         params.intres     // interlace resolution

// #pragma parameter downsample_levelx "          Downsampling-X (High-res content, pre-scalers)" 0.0 0.0 2.0 0.25

#define downsample_level         HSM_DOWNSAMPLE_BLUR_SCANLINE_DIR     // downsample level

// #pragma parameter downsample_levely "          Downsampling-Y (High-res content, pre-scalers)" 0.0 0.0 2.0 0.25

// #pragma parameter iscans 	"          Interlacing (Scanline) Saturation" 0.25 0 1 0.05
// #define iscans global.iscans

// #pragma parameter vga_mode "          VGA Single/Double Scan mode" 0.0 0.0 1.0 1.0
// #define vga_mode        global.vga_mode     // interlace saturation

#pragma parameter hiscan "          High Resolution Scanlines (ScaleFx or XBR Only)" 0.0 0.0 1.0 1.0
#define hiscan        global.hiscan


bool use_vert_scanlines_bool = false;
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
layout(set = 0, binding = 2) uniform sampler2D PrePass;
layout(set = 0, binding = 3) uniform sampler2D InfoCachePass;
layout(set = 0, binding = 4) uniform sampler2D DerezedPass;

#define COMPAT_TEXTURE(c,d) texture(c,d)


vec3 plant (vec3 tar, float r)
{
	float t = max(max(tar.r,tar.g),tar.b) + 0.00001;
	return tar * r / t;
}


vec3 fetch_pixel(vec2 coord)
{
	float downsample_levelx = use_vert_scanlines_bool ? HSM_DOWNSAMPLE_BLUR_OPPOSITE_DIR : HSM_DOWNSAMPLE_BLUR_SCANLINE_DIR;
	float downsample_levely = use_vert_scanlines_bool ? HSM_DOWNSAMPLE_BLUR_SCANLINE_DIR : HSM_DOWNSAMPLE_BLUR_OPPOSITE_DIR;
	vec2 dx = vec2(global.DerezedPassSize.z, 0.0) * downsample_levelx;
	vec2 dy = vec2(0.0, global.DerezedPassSize.w) * downsample_levely;
	vec2 d1 = dx + dy;
	vec2 d2 = dx - dy;
	
	float sum = 15.0;
	vec3 result = 3.0*COMPAT_TEXTURE(PrePass, coord     ).rgb + 
	              2.0*COMPAT_TEXTURE(PrePass, coord + dx).rgb +
	              2.0*COMPAT_TEXTURE(PrePass, coord - dx).rgb +
	              2.0*COMPAT_TEXTURE(PrePass, coord + dy).rgb +
	              2.0*COMPAT_TEXTURE(PrePass, coord - dy).rgb +
	              COMPAT_TEXTURE(PrePass, coord + d1).rgb +
	              COMPAT_TEXTURE(PrePass, coord - d1).rgb +
	              COMPAT_TEXTURE(PrePass, coord + d2).rgb +
	              COMPAT_TEXTURE(PrePass, coord - d2).rgb;  
				  
	return result/sum;
}

// noise function:
// Dedicated to the public domain.
// If you want a real license, you may consider this MIT/BSD/CC0/WTFPL-licensed (take your pick).
// Adapted from ChuckNorris - shadertoy: https://www.shadertoy.com/view/XtK3Dz

vec3 noise(vec3 v, float noise_amount){
    if (noise_amount < 0.0) v.z = -noise_amount; else v.z = mod(v.z,6001.0)/1753.0;
	// ensure reasonable range
    v = fract(v) + fract(v*1e4) + fract(v*1e-4);
    // seed
    v += vec3(0.12345, 0.6789, 0.314159);
    // more iterations => more random
    v = fract(v*dot(v, v)*123.456);
    v = fract(v*dot(v, v)*123.456);
	v = fract(v*dot(v, v)*123.456);
	v = fract(v*dot(v, v)*123.456);	
    return v;
}

void main()
{
	/* HSM Removed
	vec3 c1 = COMPAT_TEXTURE(PrePass, vTexCoord).rgb;
	vec3 c2 = COMPAT_TEXTURE(PrePass, vTexCoord + vec2(0.0, params.OriginalSize.w)).rgb;
	*/

	vec3  c1 = vec3(0);
	vec3  c2 = vec3(0);
	
	// HSM Added
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	use_vert_scanlines_bool = (USE_VERTICAL_SCANLINES > 0.5);

	// HSM Added
	if (abs(HSM_ROTATE_CORE_IMAGE) > 0.5)
	{
		use_vert_scanlines_bool = !use_vert_scanlines_bool;
	}

	vec2 rotated_derezed_size = HSM_GetRotatedDerezedSize();

	float derezed_opposite_dir_res = use_vert_scanlines_bool ? rotated_derezed_size.x : rotated_derezed_size.y;
	float derezed_scaled_opposite_dir_res = derezed_opposite_dir_res * HSM_CORE_RES_SAMPLING_MULT_OPPOSITE_DIR;

	float orig_opposite_dir_res = use_vert_scanlines_bool ? ROTATED_CORE_PREPPED_SIZE.x : ROTATED_CORE_PREPPED_SIZE.y;

	float tex_coord_scan_dir = use_vert_scanlines_bool ? vTexCoord.x : vTexCoord.y;

	vec2 downsample_offset = vec2(0, 1 / orig_opposite_dir_res / NEGATIVE_CROP_EXPAND_MULTIPLIER );

	if (use_vert_scanlines_bool)
		downsample_offset = vec2(1 / orig_opposite_dir_res / NEGATIVE_CROP_EXPAND_MULTIPLIER, 0 );

	if ((HSM_DOWNSAMPLE_BLUR_SCANLINE_DIR + HSM_DOWNSAMPLE_BLUR_OPPOSITE_DIR) > 0.0)
	{
		c1 = fetch_pixel(vTexCoord);
		c2 = fetch_pixel(vTexCoord + downsample_offset);	
	}
	else
	{
		c1 = COMPAT_TEXTURE(PrePass, vTexCoord).rgb;
		c2 = COMPAT_TEXTURE(PrePass, vTexCoord + downsample_offset).rgb;
	}
	vec3  c  = c1;

	float intera = 1.0;
	float gamma_in = clamp(GAMMA_INPUT, 1.0, 5.0);

	float m1 = max(max(c1.r,c1.g),c1.b);
	float m2 = max(max(c2.r,c2.g),c2.b);
	vec3 df = abs(c1-c2);
		
	float d = max(max(df.r,df.g),df.b);
	if (interm == 2.0) d = mix(0.1*d,10.0*d, step(m1/(m2+0.0001),m2/(m1+0.0001)));

	float r = m1;

	/* HSM Removed
	 float yres_div = 1.0; if (intres > 1.25) yres_div = intres;

	if (inter <= params.OriginalSize.y/yres_div && interm > 0.5 && intres != 1.0  && intres != 0.5 && vga_mode < 0.5)
	*/

	bool hscan = (hiscan > 0.5);
		
	// HSM Added
	if (HSM_INTERLACE_TRIGGER_RES <= derezed_scaled_opposite_dir_res && interm > 0.5 && HSM_CORE_RES_SAMPLING_MULT_OPPOSITE_DIR != 1.0 && vga_mode < 0.5 || hscan) 
	{
		intera = 0.25;

		/* HSM Removed
		float line_no  = clamp(floor(mod(params.OriginalSize.y*vTexCoord.y, 2.0)), 0.0, 1.0);
		*/

		// HSM Added
		float line_no  = clamp( floor( mod( orig_opposite_dir_res * tex_coord_scan_dir, 2.0 ) ), 0.0, 1.0 );
		float frame_no = clamp(floor(mod(float(params.FrameCount),2.0)), 0.0, 1.0);
		float ii = abs(line_no-frame_no);
		
		if (interm < 3.5)
		{
			c2 = plant(mix(c2, c2*c2, iscans), max(max(c2.r,c2.g),c2.b));
			r = clamp(max(m1*ii, (1.0-iscan)*min(m1,m2)), 0.0, 1.0);
			c = plant( mix(mix(c1,c2, min(mix(m1, 1.0-m2, min(m1,1.0-m1))/(d+0.00001),1.0)), c1, ii), r);
			if (interm == 3.0) c = (1.0-0.5*iscan)*mix(c2, c1, ii);
			intera = 0.0;
		}
		if (interm == 4.0) { c = plant(mix(c, c*c, 0.5*iscans), max(max(c.r,c.g),c.b)) * (1.0-0.5*iscan); }
		if (interm == 5.0) { c = mix(c2, c1, 0.5); c = plant(mix(c, c*c, 0.5*iscans), max(max(c.r,c.g),c.b)) * (1.0-0.5*iscan); }
	
		if (hscan) c = c1;	
	}

	#ifndef DO_NOT_LINEARIZE
	// Linearize Color
	c = pow(c, vec3(gamma_in));
	#endif

	// HSM Added Noise based on Guest.r's noise from deconvergence pass
	if (HSM_SIGNAL_NOISE_ON > 0.5)
	{
		float rc = 0.6*sqrt(max(max(c.r, c.g), c.b))+0.4;

		vec2 upscale_mult = global.SourceSize.xy / global.DerezedPassSize.xy;
		
		float noise_size = 1;

		// Scanline sized noise
		if (HSM_SIGNAL_NOISE_SIZE_MODE == 0)
			noise_size = upscale_mult.x * HSM_SIGNAL_NOISE_SIZE_MULT / SAMPLING_OPPOSITE_DIR_MULT;
		// Core resolution sized noise
		if (HSM_SIGNAL_NOISE_SIZE_MODE == 1)
			noise_size = upscale_mult.x * HSM_SIGNAL_NOISE_SIZE_MULT;
		// Uprezed sized noise
		if (HSM_SIGNAL_NOISE_SIZE_MODE == 2)
			noise_size = HSM_SIGNAL_NOISE_SIZE_MULT;

		vec3 noise0 = noise(vec3(floor(global.SourceSize.xy * vTexCoord / noise_size), float(global.FrameCount)), HSM_SIGNAL_NOISE_AMOUNT);		
		noise0 = clamp(HSM_SIGNAL_NOISE_BLACK_LEVEL * (noise0 - 0.5) + 0.5, 0, 1);
		if (HSM_SIGNAL_NOISE_TYPE < 0.5)
			c = mix(c, 				
					noise0,
					0.25 * abs(HSM_SIGNAL_NOISE_AMOUNT) * rc);
		else 
			c = min(c * mix(1.0,
							1.5 * noise0.x, 0.5 * abs(HSM_SIGNAL_NOISE_AMOUNT)),
							1.0);
	}

	if (vTexCoord.x > 0.5) gamma_in = intera; else gamma_in = 1.0/gamma_in;
	
	FragColor = vec4(c, gamma_in);
}
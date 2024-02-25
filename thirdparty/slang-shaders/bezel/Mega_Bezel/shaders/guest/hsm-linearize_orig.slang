#version 450

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

layout(push_constant) uniform Push
{
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float GAMMA_INPUT;
	float inter;
	float interm;
	float iscan;
	float intres;
	float iscans;
	float downsample_levelx;
	float downsample_levely;
	float gamma_out;
	float vga_mode;
	float hiscan;
} params;


layout(std140, set = 0, binding = 0) uniform UBO
{
	vec4 SourceSize;
	mat4 MVP;
} global;


#pragma parameter bogus_gamma "[ GAMMA OPTIONS ]: " 0.0 0.0 0.0 1.0

#pragma parameter GAMMA_INPUT "          Gamma Input" 2.4 1.0 5.0 0.05
#define GAMMA_INPUT params.GAMMA_INPUT

#pragma parameter gamma_out "          Gamma out" 2.4 1.0 5.0 0.05
#define gamma_out    params.gamma_out     // output gamma

#pragma parameter bogus_interlacing "[ INTERLACING OPTIONS ]: " 0.0 0.0 0.0 1.0

#pragma parameter inter "          Interlace Trigger Resolution/VGA Trigger:" 375.0 0.0 800.0 25.0
#define inter         params.inter     // interlace resolution

#pragma parameter interm "          Interlace Mode: OFF, Normal 1-3, Interpolation 4-5" 1.0 0.0 5.0 1.0
#define interm         params.interm     // interlace mode 

#pragma parameter iscan "          Interlacing Scanline Effect ('Laced brightness)" 0.20 0.0 1.0 0.05
#define iscan  params.iscan     // interlacing effect scanlining

#pragma parameter intres "          Internal Resolution Y: 0.5...y-dowsample" 0.0 0.0 6.0 0.5 // Joint parameter with main pass, values must match

#define intres         params.intres     // interlace resolution

#pragma parameter downsample_levelx "          Downsampling-X (High-res content, pre-scalers)" 0.0 0.0 3.0 0.05
#define downsample_levelx         params.downsample_levelx     // downsample level

#pragma parameter downsample_levely "          Downsampling-Y (High-res content, pre-scalers)" 0.0 0.0 3.0 0.05
#define downsample_levely         params.downsample_levely     // downsample level

#pragma parameter iscans "          Interlacing (Scanline) Saturation" 0.25 0.0 1.0 0.05
#define iscans        params.iscans     // interlace saturation

#pragma parameter vga_mode "          VGA Single/Double Scan mode" 0.0 0.0 1.0 1.0
#define vga_mode        params.vga_mode     // interlace saturation

#pragma parameter hiscan "          High Resolution Scanlines (prepend a scaler...)" 0.0 0.0 1.0 1.0
#define hiscan        params.hiscan


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

#define COMPAT_TEXTURE(c,d) texture(c,d)


vec3 plant (vec3 tar, float r)
{
	float t = max(max(tar.r,tar.g),tar.b) + 0.00001;
	return tar * r / t;
}


vec3 fetch_pixel(vec2 coord)
{
	vec2 dx = vec2(global.SourceSize.z, 0.0) * downsample_levelx;
	vec2 dy = vec2(0.0, global.SourceSize.w) * downsample_levely;	
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


void main()
{
	vec3 c1 = COMPAT_TEXTURE(PrePass, vTexCoord).rgb;
	vec3 c2 = COMPAT_TEXTURE(PrePass, vTexCoord + vec2(0.0, params.OriginalSize.w)).rgb;
	
	if ((downsample_levelx + downsample_levely) > 0.025)
	{
		c1 = fetch_pixel(vTexCoord);
		c2 = fetch_pixel(vTexCoord + vec2(0.0, params.OriginalSize.w));	
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

	float yres_div = 1.0; if (intres > 1.25) yres_div = intres;

	bool hscan = (hiscan > 0.5);
		
	if (inter <= params.OriginalSize.y/yres_div && interm > 0.5 && intres != 1.0  && intres != 0.5 && vga_mode < 0.5 || hscan)
	{
		intera = 0.25;
		float line_no  = clamp(floor(mod(params.OriginalSize.y*vTexCoord.y, 2.0)), 0.0, 1.0);
		float frame_no = clamp(floor(mod(float(params.FrameCount),2.0)), 0.0, 1.0);
		float ii = abs(line_no-frame_no);
		
		if (interm < 3.5)
		{
			c2 = plant(mix(c2, c2*c2, iscans), max(max(c2.r,c2.g),c2.b));
			r = clamp(max(m1*ii, (1.0-iscan)*min(m1,m2)), 0.0, 1.0);
			c = plant( mix(mix(c1,c2, min(mix(m1, 1.0-m2, min(m1,1.0-m1))/(d+0.00001),1.0)), c1, ii), r);
			if (interm == 3.0) c = (1.0-0.5*iscan)*mix(c2, c1, ii);
		}
		if (interm == 4.0) { c = plant(mix(c, c*c, 0.5*iscans), max(max(c.r,c.g),c.b)) * (1.0-0.5*iscan); }
		if (interm == 5.0) { c = mix(c2, c1, 0.5); c = plant(mix(c, c*c, 0.5*iscans), max(max(c.r,c.g),c.b)) * (1.0-0.5*iscan); }
	
		if (hscan) c = c1;	
	}
	
	if (vga_mode > 0.5)
	{
		c = c1;
		if (inter <= params.OriginalSize.y) intera = 0.75; else intera = 0.5;
	}	
	
	c = pow(c, vec3(gamma_in));
	
	if (vTexCoord.x > 0.5) gamma_in = intera; else gamma_in = 1.0/gamma_in;
	
	FragColor = vec4(c, gamma_in);
}
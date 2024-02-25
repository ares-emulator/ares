/*
   CRT - Guest - Nomask w. Curvature
   With work by DariusG to create a cut down extra fast version
   
   Copyright (C) 2017-2018 guest(r) - guest.r@gmail.com

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
	float brightboost;
	float saturation;
    float scanline;
    float beam_min;
    float beam_max;
    float h_sharp;
    float shadowMask;
    float masksize;
    float mcut;
    float maskDark;
    float maskLight;
    float CGWG;
    // float warpX;
    // float warpY;
    // float gamma_out;
    // float vignette;
} params;

// Parameter lines go here:
#pragma parameter GDVMINI_X_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter gdv_mini_title 	"[ GDV MINI - DariusG ]:" 0.0 0.0 1.0 1.0

#pragma parameter brightboost 		"          Bright boost -- brightboost" 1.0 0.5 2.0 0.05
#define brightboost params.brightboost

#pragma parameter saturation 		"          Saturation adjustment -- saturation" 1.0 0.0 2.0 0.05
#define saturation params.saturation

#pragma parameter scanline          "          Scanline Adjust -- scanline" 8.0 1.0 12.0 1.0
#define scanline params.scanline

#pragma parameter beam_min          "          Scanline Dark -- beam_min" 1.7 0.5 3.0 0.05
#define beam_min params.beam_min

#pragma parameter beam_max          "          Scanline Bright -- beam_max" 2.1 0.5 3.0 0.05
#define beam_max params.beam_max

#pragma parameter h_sharp           "          Horizontal Sharpness -- h_sharp" 2.0 1.0 5.0 0.05
#define h_sharp params.h_sharp

#pragma parameter shadowMask 		"          CRT Mask: 0:CGWG, 1-4:Lottes, 5-6:Trinitron" 0.0 -1.0 10.0 1.0
#define shadowMask params.shadowMask

#pragma parameter masksize 			"          Mask Size - 0:Auto | 2K | 4k -- masksize" 1.0 0.0 4 1.0
#define masksize     global.masksize     // Mask Size

#pragma parameter mcut 				"          Mask 5-7-10 cutoff -- mcut" 0.2 0.0 0.5 0.05
#define mcut params.mcut

#pragma parameter maskDark          "          Lottes maskDark" 0.5 0.0 2.0 0.1
#define maskDark params.maskDark

#pragma parameter maskLight 		"          Lottes maskLight" 1.5 0.0 2.0 0.1
#define maskLight params.maskLight

#pragma parameter CGWG              "          CGWG Mask Str. -- CGWG" 0.4 0.0 1.0 0.1
#define CGWG params.CGWG

// #pragma parameter warpX 			"CurvatureX (default 0.03)" 0.0 0.0 0.25 0.01
// #define warpX        params.warpX     

// #pragma parameter warpY 			"CurvatureY (default 0.04)" 0.0 0.0 0.25 0.01
// #define warpY        params.warpY     

// #pragma parameter vignette 			"Vignette On/Off" 0.0 0.0 1.0 1.0
// #define vignette 	params.vignette

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float maskFade;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;

layout(set = 0, binding = 2) uniform sampler2D InfoCachePass;

float sw(float x, float l)
{
	float d = x;
	float bm = scanline;
	float b = mix(beam_min,beam_max,l);
	d = exp2(-bm*pow(d,b));
	return d;
}

// Shadow mask (1-4 from PD CRT Lottes shader).
vec3 Mask(vec2 pos, vec3 c)
{
		// HSM Added
	float final_mask_size = masksize;

	// If using automatic Mask Size
	if (masksize < 0.5)
		final_mask_size = global.FinalViewportSize.y > 2000 ? 2 : 1;
	
	final_mask_size = max(1, floor(((1 - HSM_VIEWPORT_ZOOM_MASK) + HSM_VIEWPORT_ZOOM_MASK * VIEWPORT_SCALE.y) * final_mask_size + 0.2));

	pos = floor(pos / final_mask_size);
	vec3 mask = vec3(maskDark, maskDark, maskDark);
	
	// No mask
	if (shadowMask == -1.0)
	{
		mask = vec3(1.0);
	}       
	
	// Phosphor.
	else if (shadowMask == 0.0)
	{
		pos.x = fract(pos.x*0.5);
		float mc = 1.0 - CGWG;
		if (pos.x < 0.5) { mask.r = 1.1; mask.g = mc; mask.b = 1.1; }
		else { mask.r = mc; mask.g = 1.1; mask.b = mc; }
	}    
   
	// Very compressed TV style shadow mask.
	else if (shadowMask == 1.0)
	{
		float line = maskLight;
		float odd  = 0.0;

		if (fract(pos.x/6.0) < 0.5)
			odd = 1.0;
		if (fract((pos.y + odd)/2.0) < 0.5)
			line = maskDark;

		pos.x = fract(pos.x/3.0);
    
		if      (pos.x < 0.333) mask.b = maskLight;
		else if (pos.x < 0.666) mask.g = maskLight;
		else                    mask.r = maskLight;
		
		mask*=line;  
	} 

	// Aperture-grille.
	else if (shadowMask == 2.0)
	{
		pos.x = fract(pos.x/3.0);

		if      (pos.x < 0.333) mask.b = maskLight;
		else if (pos.x < 0.666) mask.g = maskLight;
		else                    mask.r = maskLight;
	} 

	// Stretched VGA style shadow mask (same as prior shaders).
	else if (shadowMask == 3.0)
	{
		pos.x += pos.y*3.0;
		pos.x  = fract(pos.x/6.0);

		if      (pos.x < 0.333) mask.b = maskLight;
		else if (pos.x < 0.666) mask.g = maskLight;
		else                    mask.r = maskLight;
	}

	// VGA style shadow mask.
	else if (shadowMask == 4.0)
	{
		pos.xy = floor(pos.xy*vec2(1.0, 0.5));
		pos.x += pos.y*3.0;
		pos.x  = fract(pos.x/6.0);

		if      (pos.x < 0.333) mask.b = maskLight;
		else if (pos.x < 0.666) mask.g = maskLight;
		else                    mask.r = maskLight;
	}
	
	// Alternate mask 5
	else if (shadowMask == 5.0)
	{
		float mx = max(max(c.r,c.g),c.b);
		vec3 maskTmp = vec3( min( 1.25*max(mx-mcut,0.0)/(1.0-mcut) ,maskDark + 0.2*(1.0-maskDark)*mx));
		float adj = 0.80*maskLight - 0.5*(0.80*maskLight - 1.0)*mx + 0.75*(1.0-mx);	
		mask = maskTmp;
		pos.x = fract(pos.x/2.0);
		if  (pos.x < 0.5)
		{	mask.r  = adj;
			mask.b  = adj;
		}
		else     mask.g = adj;
	}    

	// Alternate mask 6
	else if (shadowMask == 6.0)
	{
		float mx = max(max(c.r,c.g),c.b);
		vec3 maskTmp = vec3( min( 1.33*max(mx-mcut,0.0)/(1.0-mcut) ,maskDark + 0.225*(1.0-maskDark)*mx));
		float adj = 0.80*maskLight - 0.5*(0.80*maskLight - 1.0)*mx + 0.75*(1.0-mx);
		mask = maskTmp;
		pos.x = fract(pos.x/3.0);
		if      (pos.x < 0.333) mask.r = adj;
		else if (pos.x < 0.666) mask.g = adj;
		else                    mask.b = adj; 
	}
	
	// Alternate mask 7
	else if (shadowMask == 7.0)
	{
		float mc = 1.0 - CGWG;
		float mx = max(max(c.r,c.g),c.b);
		float maskTmp = min(1.6*max(mx-mcut,0.0)/(1.0-mcut) , mc);
		mask = vec3(maskTmp);
		pos.x = fract(pos.x/2.0);
		if  (pos.x < 0.5) mask = vec3(1.0 + 0.6*(1.0-mx));
	}    
	else if (shadowMask == 8.0)
	{
		float line = maskLight;
		float odd  = 0.0;

		if (fract(pos.x/4.0) < 0.5)
			odd = 1.0;
		if (fract((pos.y + odd)/2.0) < 0.5)
			line = maskDark;

		pos.x = fract(pos.x/2.0);
    
		if  (pos.x < 0.5) {mask.r = maskLight; mask.b = maskLight;}
		else  mask.g = maskLight;	
		mask*=line;  
	} 
	
	else if (shadowMask == 9.0)
    {
        vec3 Mask = vec3(maskDark);

        float bright = maskLight;
        float left  = 0.0;
      

        if (fract(pos.x/6.0) < 0.5)
            left = 1.0;
          
            
        float m = fract(pos.x/3.0);
    
        if      (m < 0.3333) Mask.b = 0.9;
        else if (m < 0.6666) Mask.g = 0.9;
        else                 Mask.r = 0.9;
        
        if      (mod(pos.y,2.0)==1.0 && left == 1.0 || mod(pos.y,2.0)==0.0 && left == 0.0 ) Mask*=bright; 
        
        return Mask; 
    } 
    
	 else if (shadowMask == 10.0)
    {
        vec3 Mask = vec3(maskDark);
        float line = maskLight;
		float odd  = 0.0;

		if (fract(pos.x/6.0) < 0.5)
			odd = 1.0;
		if (fract((pos.y + odd)/2.0) < 0.5)
			line = 1.0;    
    
        float m = fract(pos.x/3.0);
        float y = fract(pos.y/2.0);
        
        if      (m > 0.3333)  {Mask.r = 1.0; Mask.b = 1.0;}
        else if (m > 0.6666) Mask.g = 1.0;
        else                 Mask = vec3(mcut);
        if (m>0.333) Mask*=line; 
        return Mask; 
    } 
	return mask;
} 

/* HSM Removed
mat3 vign( float l )
{
    vec2 vpos = vTexCoord;

    vpos *= 1.0 - vpos.xy;
    float vig = vpos.x * vpos.y * 40.0;
    vig = min(pow(vig, 0.2), 1.0); 
    if (vignette == 0.0) vig=1.0;
   
    return mat3(vig, 0, 0,
                 0,   vig, 0,
                 0,    0, vig);

}

// Distortion of scanlines, and end of screen alpha.
vec2 Warp(vec2 pos)
{
	pos  = pos*2.0-1.0;    
    pos *= vec2(1.0 + (pos.y*pos.y)*warpX, 1.0 + (pos.x*pos.x)*warpY);
	return pos*0.5 + 0.5;
}
*/

vec3 saturationFunction (vec3 textureColor)
{
    float lum=length(textureColor.rgb)*0.5775;

    vec3 luminanceWeighting = vec3(0.3,0.6,0.1);
    if (lum<0.5) luminanceWeighting.rgb=(luminanceWeighting.rgb*luminanceWeighting.rgb)+(luminanceWeighting.rgb*luminanceWeighting.rgb);

    float luminance = dot(textureColor.rgb, luminanceWeighting);
    vec3 greyScaleColor = vec3(luminance);

    vec3 res = vec3(mix(greyScaleColor, textureColor.rgb, saturation));
    return res;
}

void main()
{
    // HSM Added
	vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	vec2 cache_bounds_coord = SCREEN_COORD;

// If it's the potato preset render the whole frame
#ifndef IS_POTATO_PRESET
#ifndef IS_NO_REFLECT_PRESET
	vec2 bezel_outside_flat_coord;
	vec2 frame_outside_flat_coord;
	HSM_GetSimpleBezelCoords(TUBE_DIFFUSE_COORD, 
							TUBE_DIFFUSE_SCALE, 
							TUBE_SCALE, 
							SCREEN_ASPECT,
							bezel_outside_flat_coord, 
							frame_outside_flat_coord);
	cache_bounds_coord = (bezel_outside_flat_coord - 0.5) * 0.9 + 0.5;
#endif
#endif

	if (HHLP_IsOutsideCoordSpace(cache_bounds_coord))
	{
			FragColor = vec4(0);
			return;
	}
	
	vec2 screen_curved_coord = HSM_GetCRTShaderCurvedCoord(SCREEN_COORD);
	vec2 pos = HSM_GetMirrorWrappedCoord(screen_curved_coord);

	// HSM Added
	vec4 SourceSize = vec4(CROPPED_ROTATED_SIZE_WITH_RES_MULT, 1/CROPPED_ROTATED_SIZE_WITH_RES_MULT);

	vec2 ps = SourceSize.zw;
	vec2 OGL2Pos = pos * SourceSize.xy;
	vec2 fp = fract(OGL2Pos);
	vec2 dx = vec2(ps.x,0.0);
	vec2 dy = vec2(0.0, ps.y);

	vec2 pC4 = floor(OGL2Pos) * ps + 0.5*ps;
	
	// Reading the texels
	vec3 ul = HSM_GetCroppedTexSample(Source, pC4     ).xyz; ul*=ul;
	vec3 ur = HSM_GetCroppedTexSample(Source, pC4 + dx).xyz; ur*=ur;
	vec3 dl = HSM_GetCroppedTexSample(Source, pC4 + dy).xyz; dl*=dl;
	vec3 dr = HSM_GetCroppedTexSample(Source, pC4 + ps).xyz; dr*=dr;
	
	float lx = fp.x;        lx = pow(lx, h_sharp);
	float rx = 1.0 - fp.x;  rx = pow(rx, h_sharp);
	
	vec3 color1 = (ur*lx + ul*rx)/(lx+rx);
	vec3 color2 = (dr*lx + dl*rx)/(lx+rx);

// calculating scanlines
	
	float f = fp.y;
	float luma1 = length(color1)*0.57735;
	float luma2 = length(color2)*0.57735;
	
	vec3 color = color1*sw(f,luma1) + color2*sw(1.0-f,luma2);
	
	color = min(color, 1.0);
	color = color * Mask(vTexCoord * global.OutputSize.xy, color);
	float lum = color.r*0.3+color.g*0.6+color.b*0.1;

	// Apply difference between GAMMA_INPUT and HSM_GAMMA_OUT_CRT
    color = pow(color, vec3(GAMMA_INPUT / HSM_GAMMA_OUT_CRT) + 0.05);
	color*= mix(1.0,brightboost,lum);

	color = saturationFunction(color);

	// HSM Removed
	// color*= vign(lum);

    FragColor = vec4(color, 1.0);
}

#version 450

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
	float sat;
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
    float csize;
    float warpX;
    float warpY;
    float gamma_out;
    float vignette;
} params;

// Parameter lines go here:
#pragma parameter gdv_mini_title 	"[ GDV MINI - DariusG ]:" 0.0 0.0 1.0 1.0
#pragma parameter brightboost 		"Bright boost" 1.0 0.5 2.0 0.05
#pragma parameter sat 				"Saturation adjustment" 1.0 0.0 2.0 0.05
#pragma parameter scanline          "Scanline Adjust" 8.0 1.0 12.0 1.0
#pragma parameter beam_min          "Scanline Dark" 1.35 0.5 3.0 0.05
#pragma parameter beam_max          "Scanline Bright" 1.05 0.5 3.0 0.05
#pragma parameter h_sharp           "Horizontal Sharpness" 2.0 1.0 5.0 0.05
#pragma parameter gamma_out 		"Gamma out" 0.5 0.2 0.6 0.01
#pragma parameter shadowMask 		"CRT Mask: 0:CGWG, 1-4:Lottes, 5-6:Trinitron" 0.0 -1.0 10.0 1.0
#pragma parameter masksize 			"CRT Mask Size (2.0 is nice in 4k)" 1.0 1.0 2.0 1.0
#pragma parameter mcut 				"Mask 5-7-10 cutoff" 0.2 0.0 0.5 0.05
#pragma parameter maskDark          "Lottes maskDark" 0.5 0.0 2.0 0.1
#pragma parameter maskLight 		"Lottes maskLight" 1.5 0.0 2.0 0.1
#pragma parameter CGWG              "CGWG Mask Str." 0.3 0.0 1.0 0.1
#pragma parameter warpX 			"CurvatureX (default 0.03)" 0.0 0.0 0.25 0.01
#pragma parameter warpY 			"CurvatureY (default 0.04)" 0.0 0.0 0.25 0.01
#pragma parameter vignette 			"Vignette On/Off" 0.0 0.0 1.0 1.0

#define gamma_out params.gamma_out
#define brightboost params.brightboost
#define sat params.sat
#define scanline params.scanline
#define beam_min params.beam_min
#define beam_max params.beam_max
#define h_sharp params.h_sharp
#define shadowMask params.shadowMask
#define masksize params.masksize
#define mcut params.mcut
#define maskDark params.maskDark
#define maskLight params.maskLight
#define CGWG 		params.CGWG
#define warpX        params.warpX     
#define warpY        params.warpY     
#define vignette 	params.vignette

#define lumweight vec3(0.3,0.6,0.1)


layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 FinalViewportSize;
	vec4 OutputSize;
} global;

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

float sw (float y, float l,float v)
{
    float scan = mix(scanline-2.0, scanline, y);
    float tmp = mix(beam_min+v, beam_max+v, l);
    float ex = y*tmp;
    return exp2(-scan*ex*ex);
}

// Shadow mask (1-4 from PD CRT Lottes shader).
vec3 Mask(vec2 pos, vec3 c)
{
	pos = floor(pos / masksize);
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

mat3 vign( float l )
{
    vec2 vpos = vTexCoord;

    vpos *= 1.0 - vpos.xy;
    float vig = vpos.x * vpos.y * 45.0;
    vig = min(pow(vig, 0.15), 1.0); 
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


void main()
{
	vec2 pos = Warp(vTexCoord);	

	vec2 OGL2Pos = pos * params.SourceSize.xy;
	vec2 fp = fract(OGL2Pos);
	vec2 ps = params.SourceSize.zw;
	vec2 dx = vec2(ps.x,0.0);
	vec2 dy = vec2(0.0, ps.y);

	vec2 pC4 = floor(OGL2Pos) * ps + 0.5*ps;	
	float f = fp.y; if (params.OriginalSize.y > 400.0) f=1.0;
	vec3 color;

	// Reading the texels
	vec3 ul = texture(Source, pC4     ).xyz; 
	vec3 ur = texture(Source, pC4 - dx).xyz; 
	vec3 dl = texture(Source, pC4 + dy).xyz; 
	vec3 dr = texture(Source, pC4 + vec2(-ps.x,ps.y)).xyz; 
	
	float lx = fp.x;        lx = pow(lx, h_sharp);
	float rx = 1.0 - fp.x;  rx = pow(rx, h_sharp);
	
	vec3 color1 = (ur*rx + ul*lx)/(lx+rx);  
	vec3 color2 = (dr*rx + dl*lx)/(lx+rx);	
	color1 *=color1; color2 *=color2;

// calculating scanlines
	float lum = dot (color1, lumweight);
	float lum2 = dot (color2, lumweight);


  float v = 0.0;
  // vignette
  if (vignette > 0.0){  
  v = (vTexCoord.x-0.5);  // range -0.5 to 0.5, 0.0 being center of screen
  v = v*v;    	     // curved response: higher values (more far from center) get higher results.
}
    color = color1*sw(f, lum,v) + color2*sw(1.0-f, lum2,v); 
	color = color*Mask(vTexCoord*params.OutputSize.xy, color);

	color = pow(color, vec3(gamma_out));
	float s = dot(lumweight,color);
	color*= mix(1.0,brightboost,lum);
	color = mix(vec3(s),color,sat);

    FragColor.rgb = color;
} 

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

	float brightboost;
	float sat;
	float glow;
    float Size;
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
    float gamma_out_red;
    float gamma_out_green;
    float gamma_out_blue;
    float vignette;
    float gdv_mono;
	float gdv_R;
	float gdv_G;
	float gdv_B;
	float thres;
} params;

// Parameter lines go here:

#pragma parameter scanline          "Scanline Adjust" 10.0 1.0 15.0 1.0
#define scanline params.scanline

#pragma parameter beam_min          "Scanline Dark" 1.5 0.5 3.0 0.05
#define beam_min params.beam_min

#pragma parameter beam_max          "Scanline Bright" 2.0 0.5 3.0 0.05
#define beam_max params.beam_max

#pragma parameter h_sharp           "Horizontal Sharpness" 2.5 1.0 5.0 0.05
#define h_sharp params.h_sharp

#pragma parameter shadowMask 		"  CRT Mask: 0:CGWG, 1-4:Lottes, 5-6:Trinitron" 11.0 -1.0 11.0 1.0
#define shadowMask params.shadowMask

#pragma parameter thres "  Mask Effect Threshold" 0.4 0.0 0.9 0.02
#define thres params.thres 

#pragma parameter masksize 			"  CRT Mask Size (2.0 is nice in 4k)" 1.0 1.0 2.0 1.0
#define masksize params.masksize

#pragma parameter mcut 				"  Mask 5-7-10 cutoff" 0.2 0.0 0.5 0.05
#define mcut params.mcut

#pragma parameter maskDark          "  Lottes maskDark" 0.0 0.0 2.0 0.1
#define maskDark params.maskDark

#pragma parameter maskLight 		"  Lottes maskLight" 1.5 0.0 2.0 0.1
#define maskLight params.maskLight

#pragma parameter CGWG              "  CGWG Mask Str." 1.0 0.0 1.0 0.1
#define CGWG 		params.CGWG

#pragma parameter warpX 			"CurvatureX (default 0.03)" 0.0 0.0 0.25 0.01
#define warpX        params.warpX     

#pragma parameter warpY 			"CurvatureY (default 0.04)" 0.05 0.0 0.25 0.01
#define warpY        params.warpY     

#pragma parameter vignette 			"Vignette On/Off" 1.0 0.0 1.0 1.0
#define vignette 	params.vignette

#pragma parameter gamma_out_red 		"  Gamma out Red" 2.2 1.0 4.0 0.1
#define gamma_out_red params.gamma_out_red

#pragma parameter gamma_out_green 		"  Gamma out Green" 2.2 1.0 4.0 0.1
#define gamma_out_green params.gamma_out_green

#pragma parameter gamma_out_blue 		"  Gamma out Blue" 2.2 1.0 4.0 0.1
#define gamma_out_blue params.gamma_out_blue

#pragma parameter brightboost 		"  Bright boost" 1.2 0.5 2.0 0.05
#define brightboost params.brightboost

#pragma parameter sat 				"  Saturation adjustment" 1.2 0.0 2.0 0.05
#define sat params.sat

#pragma parameter glow "Glow Strength" 0.35 0.0 1.0 0.01
#define glow params.glow


#pragma parameter gdv_mono "Mono Display On/Off" 0.0 0.0 1.0 1.0
#define gdv_mono params.gdv_mono

#pragma parameter gdv_R "Mono Red/Channel" 1.0 0.0 2.0 0.01
#define gdv_R params.gdv_R

#pragma parameter gdv_G "Mono Green/Channel" 1.0 0.0 2.0 0.01
#define gdv_G params.gdv_G

#pragma parameter gdv_B "Mono Blue/Channel" 1.0 0.0 2.0 0.01
#define gdv_B params.gdv_B



layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 FinalViewportSize;
	vec4 OutputSize;
    vec4 SourceSize;
	vec4 OriginalSize;
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

float sw(float x, float l)
{
	float d = x;
	float bm = scanline;
	float b = mix(beam_min,beam_max,l);
	d = exp2(-bm*pow(d,b));
	return d;
}

vec3 toGrayscale(vec3 color)
{
  float average = (color.r + color.g + color.b) / 3.0;
  return vec3(average);
}

vec3 colorize(vec3 grayscale, vec3 color)
{
    return (grayscale * color);
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

	else if (shadowMask == 11.0)
	{
		vec3 Mask = vec3(maskDark);
		pos.x = fract(pos.x/3.0);

		if      (pos.x > 0.333) Mask = vec3(1.0);
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

vec3 saturation (vec3 textureColor)
{
    float lum=length(textureColor.rgb)*0.5775;

    vec3 luminanceWeighting = vec3(0.3,0.6,0.1);
    if (lum<0.5) luminanceWeighting.rgb=(luminanceWeighting.rgb*luminanceWeighting.rgb)+(luminanceWeighting.rgb*luminanceWeighting.rgb);

    float luminance = dot(textureColor.rgb, luminanceWeighting);
    vec3 greyScaleColor = vec3(luminance);

    vec3 color1 = vec3(mix(greyScaleColor, textureColor.rgb, sat));
    return color1;
}

vec3 glow0 (vec2 texcoord, vec3 col)
{
   vec3 sum = vec3(0.0);
   vec2 blurSize = vec2 (global.SourceSize.zw);

   vec3 c20 = texture(Source, vec2(texcoord.x - 2.0 * blurSize.x, texcoord.y)).rgb; c20*c20;
   vec3 c10 = texture(Source, vec2(texcoord.x - blurSize.x,       texcoord.y)).rgb; c10*c10;
   vec3 c11 = texture(Source, vec2(texcoord.x,                  texcoord.y)).rgb; c11*c11;
   vec3 c12 = texture(Source, vec2(texcoord.x + blurSize.x,       texcoord.y)).rgb; c12*c12;
   vec3 c21 = texture(Source, vec2(texcoord.x + 2.0 * blurSize.x, texcoord.y)).rgb; c21*c21;

   vec3 c22 = texture(Source, vec2(texcoord.x - 2.0 * blurSize.x, texcoord.y - blurSize.y)).rgb; c22*c22;
   vec3 c23 = texture(Source, vec2(texcoord.x - blurSize.x,       texcoord.y - 2.0 * blurSize.y)).rgb; c23*c23;
   vec3 c13 = texture(Source, vec2(texcoord.x - blurSize.x,       texcoord.y - blurSize.y)).rgb; c13*c13;
   vec3 c14 = texture(Source, vec2(texcoord.x + blurSize.x,       texcoord.y + blurSize.y)).rgb; c14*c14;
   vec3 c24 = texture(Source, vec2(texcoord.x + blurSize.x,       texcoord.y + 2.0 * blurSize.y)).rgb; c24*c24;
   vec3 c25 = texture(Source, vec2(texcoord.x + 2.0 * blurSize.x, texcoord.y + blurSize.y)).rgb; c25*c25;
   
   vec3 c26 = texture(Source, vec2(texcoord.x - 2.0 * blurSize.x, texcoord.y + blurSize.y)).rgb; c26*c26;
   vec3 c27 = texture(Source, vec2(texcoord.x - blurSize.x,       texcoord.y + 2.0 * blurSize.y)).rgb; c27*c27;
   vec3 c15 = texture(Source, vec2(texcoord.x - blurSize.x,       texcoord.y + blurSize.y)).rgb; c15*c15;
   vec3 c16 = texture(Source, vec2(texcoord.x + blurSize.x,       texcoord.y - blurSize.y)).rgb; c16*c16;
   vec3 c28 = texture(Source, vec2(texcoord.x + blurSize.x,       texcoord.y - 2.0 * blurSize.y)).rgb; c28*c28;
   vec3 c29 = texture(Source, vec2(texcoord.x + 2.0 * blurSize.x, texcoord.y - blurSize.y)).rgb; c29*c29;
   
   vec3 c30 = texture(Source, vec2(texcoord.x,                  texcoord.y - 2.0 * blurSize.y)).rgb; c30*c30;
   vec3 c17 = texture(Source, vec2(texcoord.x,                  texcoord.y - blurSize.y)).rgb; c17*c17;
   vec3 c18 = texture(Source, vec2(texcoord.x,                  texcoord.y + blurSize.y)).rgb; c18*c18;
   vec3 c31 = texture(Source, vec2(texcoord.x,                  texcoord.y + 2.0 * blurSize.y)).rgb; c31*c31;
  	sum = (3.0*c11 + 2.5 *(c10+c12+c13+c14+c15+c16+c17+c18) + 1.5*(c20+c21+c22+c23+c24+c25+c26+c27+c28+c29+c30+c31))/45.0;
   return sum * glow; 
}



void main()
{
	vec2 pos = Warp(vTexCoord);

	// HSM Added
	vec4 SourceSize = global.SourceSize;

	vec2 ps = SourceSize.zw;
	vec2 OGL2Pos = pos * SourceSize.xy;
	vec2 fp = fract(OGL2Pos);
	vec2 dx = vec2(ps.x,0.0);
	vec2 dy = vec2(0.0, ps.y);

	vec2 pC4 = floor(OGL2Pos) * ps + 0.5*ps;
	
	// Reading the texels
	vec3 ul = texture(Source, pC4     ).xyz; 
	vec3 ur = texture(Source, pC4 + dx).xyz; 
	vec3 dl = texture(Source, pC4 + dy).xyz; 
	vec3 dr = texture(Source, pC4 + ps).xyz;
	
	float lx = fp.x;        lx = pow(lx, h_sharp);
	float rx = 1.0 - fp.x;  rx = pow(rx, h_sharp);
	
	vec3 color1 = (ur*lx + ul*rx)/(lx+rx);
	vec3 color2 = (dr*lx + dl*rx)/(lx+rx);

	
	float f = fp.y;
	float luma1 = color1.r*0.3+color1.g*0.6+color1.b*0.1;
	float luma2 = color2.r*0.3+color2.g*0.6+color2.b*0.1;


	color1 = (2.0*pow(color1,vec3(2.8))) - pow(color1,vec3(3.6));
	color2 = (2.0*pow(color2,vec3(2.8))) - pow(color2,vec3(3.6));

	color1 = color1 * mix(Mask(vTexCoord * global.OutputSize.xy, color1),vec3(1.0),luma1*thres);
	color2 = color2 * mix(Mask(vTexCoord * global.OutputSize.xy, color2),vec3(1.0),luma2*thres);


	vec3 color = color1*sw(f,luma1) + color2*sw(1.0-f,luma2);
	vec4 OriginalSize = global.OriginalSize;

	if (OriginalSize.y >= 400.0) {color = (color1 + color2)/2.0;}

	color = min(color, 1.0);
	float lum = color.r*0.3+color.g*0.6+color.b*0.1;

	color = pow(color, vec3(1.0/gamma_out_red,1.0,1.0));
	color = pow(color, vec3(1.0,1.0/gamma_out_green,1.0));
	color = pow(color, vec3(1.0,1.0,1.0/gamma_out_blue));
	color += glow0(pC4,color);
	color*= mix(1.0,brightboost,lum);

	color = saturation(color);
	color*= vign(lum);


    if (gdv_mono == 1.0)
	{
    vec3 col1 = toGrayscale (color);
    vec3 c = vec3(gdv_R, gdv_G, gdv_B);
    color = colorize (col1, c);
    }

    else color = color;

	FragColor = vec4(color, 1.0);
}

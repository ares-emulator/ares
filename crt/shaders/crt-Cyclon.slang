#version 450
/*

DariusG presents

'crt-Cyclon' 

Why? Because it's speedy!

A super-fast shader based on the magnificent crt-Geom, optimized for full speed 
on a Xiaomi Note 3 Pro cellphone (around 170(?) gflops gpu or so)

This shader uses parts from:
crt-Geom (scanlines)
Quillez (main filter)
Grade (some primaries)
Dogway's inverse Gamma
Masks-slot-color handling, tricks etc are mine.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version.

*/

layout(push_constant) uniform Push
{
 float SCANLINE,INTERLACE,M_TYPE,MSIZE,SLOT,SLOTW,BGR,Maskl,Maskh,C_STR,CONV_R,CONV_G,CONV_B,
  		WARPX,WARPY,BR_DEP,c_space,EXT_GAMMA;
} params;

// Parameter lines go here:
#pragma parameter SCANLINE "Scanline Weight" 0.3 0.2 0.6 0.05
#pragma parameter INTERLACE "Interlacing On/Off" 1.0 0.0 1.0 1.0
#pragma parameter bogus_msk " [ MASK SETTINGS ] " 0.0 0.0 0.0 0.0
#pragma parameter M_TYPE "Mask Type: -1:None, 0:CGWG, 1:RGB" 1.0 -1.0 1.0 1.0
#pragma parameter MSIZE "Mask Size" 1.0 1.0 2.0 1.0
#pragma parameter SLOT "Slot Mask On/Off" 1.0 0.0 1.0 1.0
#pragma parameter SLOTW "Slot Mask Width" 3.0 2.0 3.0 1.0
#pragma parameter BGR "Subpixels BGR/RGB" 0.0 0.0 1.0 1.0
#pragma parameter Maskl "Mask Brightness Dark" 0.3 0.0 1.0 0.05
#pragma parameter Maskh "Mask Brightness Bright" 0.75 0.0 1.0 0.05
#pragma parameter bogus_geom " [ GEOMETRY SETTINGS ] " 0.0 0.0 0.0 0.0
#pragma parameter bzl "Bezel On/Off" 1.0 0.0 1.0 1.0
#pragma parameter ambient "Ambient Light" 0.25 0.0 1.0 0.05
#pragma parameter zoomx "Zoom Image X" 0.0 -1.0 1.0 0.005
#pragma parameter zoomy "Zoom Image Y" 0.0 -1.0 1.0 0.005
#pragma parameter centerx "Image Center X" 0.0 -5.0 5.0 0.05 
#pragma parameter centery "Image Center Y" 0.0 -5.0 5.0 0.05
#pragma parameter WARPX "Curvature Horizontal" 0.02 0.00 0.25 0.01
#pragma parameter WARPY "Curvature Vertical" 0.01 0.00 0.25 0.01
#pragma parameter vig "Vignette On/Off" 1.0 0.0 1.0 1.0
#pragma parameter bogus_col " [ COLOR SETTINGS ] " 0.0 0.0 0.0 0.0
#pragma parameter BR_DEP "Scan/Mask Brightness Dependence" 0.2 0.0 0.333 0.01
#pragma parameter c_space "Color Space: sRGB,PAL,NTSC-U,NTSC-J" 0.0 0.0 3.0 1.0
#pragma parameter EXT_GAMMA "External Gamma In (Glow etc)" 0.0 0.0 1.0 1.0
#pragma parameter SATURATION "Saturation" 1.0 0.0 2.0 0.05
#pragma parameter BRIGHTNESS_ "Brightness, Sega fix:1.06" 1.0 0.0 2.0 0.01
#pragma parameter BLACK  "Black Level" 0.0 -0.20 0.20 0.01 
#pragma parameter RG "Green <-to-> Red Hue" 0.0 -0.25 0.25 0.01
#pragma parameter RB "Blue <-to-> Red Hue"  0.0 -0.25 0.25 0.01
#pragma parameter GB "Blue <-to-> Green Hue" 0.0 -0.25 0.25 0.01
#pragma parameter bogus_con " [ CONVERGENCE SETTINGS ] " 0.0 0.0 0.0 0.0
#pragma parameter C_STR "Convergence Overall Strength" 0.0 0.0 0.5 0.05
#pragma parameter CONV_R "Convergence Red X-Axis" 0.0 -1.0 1.0 0.05
#pragma parameter CONV_G "Convergence Green X-axis" 0.0 -1.0 1.0 0.05
#pragma parameter CONV_B "Convergence Blue X-Axis" 0.0 -1.0 1.0 0.05
#pragma parameter POTATO "Potato Boost(Simple Gamma, adjust Mask)" 0.0 0.0 1.0 1.0

  
#define pi 3.1415926535897932384626433

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
    float BLACK, RG, RB, GB, POTATO,SATURATION,BRIGHTNESS_,bzl,zoomx,zoomy,centerx,centery,vig,ambient;
    vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} global;

#define SourceSize  global.SourceSize
#define OriginalSize global.OriginalSize
#define OutputSize   global.OutputSize
#define FrameCount global.FrameCount

#define M_TYPE   params.M_TYPE
#define BGR      params.BGR
#define MSIZE    params.MSIZE
#define Maskl    params.Maskl
#define Maskh    params.Maskh     
#define C_STR    params.C_STR    
#define CONV_R   params.CONV_R     
#define CONV_G   params.CONV_G     
#define CONV_B   params.CONV_B      
#define SCANLINE params.SCANLINE   
#define INTERLACE params.INTERLACE  
#define WARPX     params.WARPX   
#define WARPY     params.WARPY  
#define SLOT      params.SLOT    
#define SLOTW     params.SLOTW    
#define c_space   params.c_space    
#define CORNER      params.CORNER  
#define B_SMOOTH    params.B_SMOOTH  
#define BR_DEP      params.BR_DEP   
#define EXT_GAMMA   params.EXT_GAMMA   
#define PAL_NTSC    params.PAL_NTSC 
#define RG global.RG   
#define RB global.RB   
#define GB global.GB   
#define BLACK global.BLACK
#define POTATO global.POTATO
#define SATURATION  global.SATURATION   
#define BRIGHTNESS_  global.BRIGHTNESS_  
#define bzl global.bzl
#define zoomx global.zoomx
#define zoomy global.zoomy
#define centerx global.centerx
#define centery global.centery
#define vig global.vig
#define ambient global.ambient

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 scale;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord * 1.0001;
    scale = SourceSize.xy/OriginalSize.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(location = 1) in vec2 scale;

layout(set = 0, binding = 1) uniform sampler2D Source;
layout(set = 0, binding = 2) uniform sampler2D bezel;


///////////////////////////////////////////////////////////////////////////////////////////////
#define blck (1.0)/(1.0-BLACK);


vec3 Mask(vec2 pos, float CGWG)
{
    vec3 mask = vec3(CGWG);
    
    
if (M_TYPE == 0.0){

    if (POTATO == 1.0)  return vec3( (1.0-CGWG)*sin(pos.x*pi)+CGWG) ;
    else{
    float m = fract(pos.x*0.5);

    if (m<0.5) mask.rb = vec2(1.0);
    else mask.g = 1.0;

    return mask;}
    }

if (M_TYPE == 1.0){

    if (POTATO == 1.0)  return vec3( (1.0-CGWG)*sin(pos.x*pi*0.6667)+CGWG) ;
    else{
    float m = fract(pos.x*0.3333);

    if (m<0.3333) mask.rgb = (BGR == 0.0) ? vec3(mask.r, mask.g, 1.0) : vec3(1.0, mask.g, mask.b);
    else if (m<0.6666)         mask.g = 1.0;
    else          mask.rgb = (BGR == 0.0) ? vec3(1.0, mask.g, mask.b) : vec3(mask.r, mask.g, 1.0);
    return mask;
    }
}
    else return vec3(1.0);

}

float scanlineWeights(float distance, vec3 color, float x)
    {
    // "wid" controls the width of the scanline beam, for each RGB
    // channel The "weights" lines basically specify the formula
    // that gives you the profile of the beam, i.e. the intensity as
    // a function of distance from the vertical center of the
    // scanline. In this case, it is gaussian if width=2, and
    // becomes nongaussian for larger widths. Ideally this should
    // be normalized so that the integral across the beam is
    // independent of its width. That is, for a narrower beam
    // "weights" should have a higher peak at the center of the
    // scanline than for a wider beam.
    float wid = SCANLINE + 0.15 * dot(color, vec3(0.25-0.8*x));   //0.8 vignette strength
    float weights = distance / wid;
    return 0.4 * exp(-weights * weights ) / wid;
    }

#define pwr vec3(1.0/((-1.0*SCANLINE+1.0)*(-0.8*CGWG+1.0))-1.2)
// Returns gamma corrected output, compensated for scanline+mask embedded gamma
vec3 inv_gamma(vec3 col, vec3 power)
{
    vec3 cir  = col-1.0;
         cir *= cir;
         col  = mix(sqrt(col),sqrt(1.0-cir),power);
    return col;
}

// standard 6500k
mat3 PAL = mat3(                    
1.0740  ,   -0.0574 ,   -0.0119 ,
0.0384  ,   0.9699  ,   -0.0059 ,
-0.0079 ,   0.0204  ,   0.9884  );

// standard 6500k
mat3 NTSC = mat3(                   
0.9318  ,   0.0412  ,   0.0217  ,
0.0135  ,   0.9711  ,   0.0148  ,
0.0055  ,   -0.0143 ,   1.0085  );

// standard 8500k
mat3 NTSC_J = mat3(                    
0.9501  ,   -0.0431 ,   0.0857  ,
0.0265  ,   0.9278  ,   0.0432  ,
0.0011  ,   -0.0206 ,   1.3153  );

vec3 slot(vec2 pos)
{
    float h = fract(pos.x/SLOTW);
    float v = fract(pos.y);
    
    float odd;
    if (v<0.5) odd = 0.0; else odd = 1.0;

if (odd == 0.0)
    {if (h<0.5) return vec3(0.5); else return vec3(1.5);}

else if (odd == 1.0)
    {if (h<0.5) return vec3(1.5); else return vec3(0.5);}
}

vec2 Warp(vec2 pos)
{
    pos = pos*2.0-1.0;
    pos *= vec2(1.0+pos.y*pos.y*WARPX, 1.0+pos.x*pos.x*WARPY);
    pos = pos*0.5+0.5;

    return pos;
}

void main()
{   

// Hue matrix inside main() to avoid GLES error
mat3 hue = mat3(
    1.0, -RG, -RB,
    RG, 1.0, -GB,
    RB, GB, 1.0
);
// zoom in and center screen for bezel
    vec2 pos = Warp((vTexCoord*vec2(1.0-zoomx,1.0-zoomy)-vec2(centerx,centery)/100.0));
    vec4 bez = vec4(0.0);
    if (bzl == 1.0) bez = texture(bezel,vTexCoord*SourceSize.xy/OriginalSize.xy*0.97+vec2(0.015,0.015));   

    bez.rgb = mix(bez.rgb, vec3(ambient),0.5);

    vec2 bpos = pos;
    vec2 ps = SourceSize.zw;
    vec2 dx = vec2(ps.x,0.0);
// Quilez
    vec2 ogl2 = pos*SourceSize.xy;
    vec2 i = floor(pos*SourceSize.xy) + 0.5;
    float f = ogl2.y - i.y;
    pos.y = (i.y + 4.0*f*f*f)*ps.y; // smooth
    pos.x = mix(pos.x, i.x*ps.x, 0.2);

// Convergence
    vec3  res0 = texture(Source,pos).rgb;
    float resr = texture(Source,pos + dx*CONV_R).r;
    float resb = texture(Source,pos + dx*CONV_B).b;
    float resg = texture(Source,pos + dx*CONV_G).g;

    vec3 res = vec3(  res0.r*(1.0-C_STR) +  resr*C_STR,
                      res0.g*(1.0-C_STR) +  resg*C_STR,
                      res0.b*(1.0-C_STR) +  resb*C_STR 
                   );
// Vignette
    float x = 0.0;
    if (vig == 1.0){
    x = vTexCoord.x*scale.x-0.5;
    x = x*x;}

    float l = dot(vec3(BR_DEP),res);
 
 // Color Spaces   
    if(EXT_GAMMA != 1.0) res *= res;
    if (c_space != 0.0) {
    if (c_space == 1.0) res *= PAL;
    if (c_space == 2.0) res *= NTSC;
    if (c_space == 3.0) res *= NTSC_J;
// Apply CRT-like luminances
    res /= vec3(0.24,0.69,0.07);
    res *= vec3(0.29,0.6,0.11); 
    res = clamp(res,0.0,1.0);
    }
    float s = fract(bpos.y*SourceSize.y-0.5);
// handle interlacing
    if (OriginalSize.y > 400.0) 
    {
        s = fract(bpos.y*SourceSize.y/2.0-0.5);
        if (INTERLACE == 1.0) s = mod(float(FrameCount),2.0) < 1.0 ? s: s+0.5;
    }
// Calculate CRT-Geom scanlines weight and apply
    float weight  = scanlineWeights(s, res, x);
    float weight2 = scanlineWeights(1.0-s, res, x);
    res *= weight + weight2;

// Masks
    vec2 xy = vTexCoord*OutputSize.xy*scale/MSIZE;    
    float CGWG = mix(Maskl, Maskh, l);
    res *= Mask(xy, CGWG);
// Apply slot mask on top of Trinitron-like mask
    if (SLOT == 1.0) res *= mix(slot(xy/2.0),vec3(1.0),CGWG);
    
    if (POTATO == 0.0) res = inv_gamma(res,pwr);
    else {res = sqrt(res); res *= mix(1.3,1.1,l);}

// Saturation
    float lum = dot(vec3(0.29,0.60,0.11),res);
    res = mix(vec3(lum),res,SATURATION);

// Brightness, Hue and Black Level
    res *= BRIGHTNESS_;
    res *= hue;
    res -= vec3(BLACK);
    res *= blck;
// Apply bezel code, adapted from New-Pixie
    if (bzl >0.0)
    res.rgb = mix(res.rgb, mix(max(res.rgb, 0.0), pow( abs(bez.rgb), vec3( 1.4 ) ), bez.w * bez.w), vec3( 1.0 ) );

    FragColor = vec4(res,1.0);
}

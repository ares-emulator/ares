#version 450

/*
   Dot Mask
   Authors: cgwg, Timothy Lottes, torridgristle
   License: GPL
   
   Note: This shader is just the dotmask functions from cgwg's CRT shader and crt-lottes plus torridgristle's mask.
*/

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   float DOTMASK_STRENGTH;
   float maskDark;
   float maskLight;
   float shadowMask;
} global;

#pragma parameter DOTMASK_STRENGTH "CGWG Dot Mask Strength" 0.3 0.0 1.0 0.01
#pragma parameter maskDark "Lottes maskDark" 0.5 0.0 2.0 0.1
#pragma parameter maskLight "Lottes maskLight" 1.5 0.0 2.0 0.1
#pragma parameter shadowMask "Mask Style" 3.0 0.0 5.0 1.0

#define mod_factor vTexCoord.x * global.SourceSize.x * global.OutputSize.x / global.SourceSize.x

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

// Lottes' public domain mask code
// Shadow mask.
vec3 Mask(vec2 pos){
  vec3 mask=vec3(global.maskDark,global.maskDark,global.maskDark);
  
// Very compressed TV style shadow mask.
  if (global.shadowMask == 1.0) {
    float line=global.maskLight;
    float odd=0.0;
    if(fract(pos.x/6.0)<0.5)odd=1.0;
    if(fract((pos.y+odd)/2.0)<0.5)line=global.maskDark;  
    pos.x=fract(pos.x/3.0);
   
    if(pos.x<0.333)mask.r=global.maskLight;
    else if(pos.x<0.666)mask.g=global.maskLight;
    else mask.b=global.maskLight;
    mask*=line;  
  } 

// Aperture-grille.
  else if (global.shadowMask == 2.0) {
    pos.x=fract(pos.x/3.0);

    if(pos.x<0.333)mask.r=global.maskLight;
    else if(pos.x<0.666)mask.g=global.maskLight;
    else mask.b=global.maskLight;
  } 

// Stretched VGA style shadow mask (same as prior shaders).
  else if (global.shadowMask == 3.0) {
    pos.x+=pos.y*3.0;
    pos.x=fract(pos.x/6.0);

    if(pos.x<0.333)mask.r=global.maskLight;
    else if(pos.x<0.666)mask.g=global.maskLight;
    else mask.b=global.maskLight;
  }

// VGA style shadow mask.
  else if (global.shadowMask == 4.0) {
    pos.xy=floor(pos.xy*vec2(1.0,0.5));
    pos.x+=pos.y*3.0;
    pos.x=fract(pos.x/6.0);

    if(pos.x<0.333)mask.r=global.maskLight;
    else if(pos.x<0.666)mask.g=global.maskLight;
    else mask.b=global.maskLight;
  }

  return mask;
}

// torridgristle's shadowmask code
const float Pi = 3.1415926536;

vec3 SinPhosphor(vec3 image)
{
    float MaskR = sin(global.OutputSize.x*vTexCoord.x*Pi*1.0+Pi*0.00000+vTexCoord.y*global.OutputSize.y*Pi*0.5)*0.5+0.5;
    float MaskG = sin(global.OutputSize.x*vTexCoord.x*Pi*1.0+Pi*1.33333+vTexCoord.y*global.OutputSize.y*Pi*0.5)*0.5+0.5;
    float MaskB = sin(global.OutputSize.x*vTexCoord.x*Pi*1.0+Pi*0.66667+vTexCoord.y*global.OutputSize.y*Pi*0.5)*0.5+0.5;
    
    vec3 Mask = vec3(MaskR,MaskG,MaskB);
    
    Mask = min(Mask*2.0,1.0);
    
    return vec3(Mask * image);
}

// cgwg's aperture grille
vec3 cgwg_mask(vec3 image)
{
   float mask = 1.0 - global.DOTMASK_STRENGTH;
   //Output pixels are alternately tinted green and magenta
   vec3 dotMaskWeights = mix(vec3(1.0, mask, 1.0),
						  vec3(mask, 1.0, mask),
						  floor(mod(mod_factor, 2.0)));
   return image * dotMaskWeights;
}

void main()
{
   vec3 res = texture(Source, vTexCoord).rgb;
      
   if (global.shadowMask == 0.0)
   {
      res = cgwg_mask(res);
      FragColor = vec4(res, 1.0);
      return;
	} 
   else if (global.shadowMask == 5.0)
   {
      res *= SinPhosphor(res);
   }
   else
   {
      // Lottes mask needs linear gamma
      res = pow(res, vec3(2.2));
      res *= Mask(floor(1.000001 * vTexCoord.xy * global.OutputSize.xy + vec2(0.5)));
   }
   FragColor = vec4(pow(res, vec3(1.0/2.2)), 1.0);
}
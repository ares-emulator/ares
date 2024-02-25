#version 450

/**
* Practical Elliptical Texture Filtering on the GPU
* Copyright 2010-2011 Pavlos Mavridis, All rights reserved.
*
* Version: 0.6 - 12 / 7 / 2011 (DD/MM/YY)
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float warpX, warpY;
   float DOTMASK_STRENGTH;
   float maskDark;
   float maskLight;
   float shadowMask;
} params;

#pragma parameter warpX "EWA Curvature X-Axis" 0.031 0.0 0.125 0.01
#pragma parameter warpY "EWA Curvature Y-Axis" 0.041 0.0 0.125 0.01
vec2 Distortion = vec2(params.warpX, params.warpY) * 15.;
#pragma parameter shadowMask "Mask Style" 3.0 -1.0 5.0 1.0
#pragma parameter DOTMASK_STRENGTH "CGWG Dot Mask Strength" 0.3 0.0 1.0 0.01
#pragma parameter maskDark "Lottes maskDark" 0.5 0.0 2.0 0.1
#pragma parameter maskLight "Lottes maskLight" 1.5 0.0 2.0 0.1

#define mod_factor vTexCoord.x * params.SourceSize.x * params.OutputSize.x / params.SourceSize.x

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

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

//{========= TEXTURE FILTERING (EWA) PARAMETERS =========
#define MAX_ECCENTRICITY 1
#define FILTER_WIDTH 0.8
#define FILTER_SHARPNESS 1.0
#define TEXELS_PER_PIXEL 1.0
#define TEXEL_LIMIT 32
#define FILTER_FUNC gaussFilter
//}======================================================

#define M_PI 3.14159265358979323846

#define SourceImage Source

//{========================= FILTER FUNCTIONS =======================
// We only use the Gaussian filter function. The other filters give
// very similar results.
 
float boxFilter(float r2){
    return 1.0;
}
 
float gaussFilter(float r2){
    float alpha = FILTER_SHARPNESS;
    return exp(-alpha * r2);
}
 
float triFilter(float r2){
    float alpha = FILTER_SHARPNESS;
    float r= sqrt(r2);
    return max(0, 1.-r);///alpha);
}
 
float sinc(float x){
    return sin(M_PI*x)/(M_PI*x);
}
 
float lanczosFilter(float r2){
    if (r2==0)
        return 1.;
    float r= sqrt(r2);
    return sinc(r)*sinc(r/1.3);
}
 
//catmull-rom filter
float crFilter(float r2){
    float r = sqrt(r2);
    return (r>=2.)?.0:(r<1.)?(3.*r*r2-5.*r2+2.):(-r*r2+5.*r2-8*r+4.);
}
 
float quadraticFilter(float r2){
    float a = FILTER_SHARPNESS;
    return 1.0 - r2/(a*a);
}
 
float cubicFilter(float r2){
    float a = FILTER_SHARPNESS;
    float r = sqrt(r2);
    return 1.0 - 3*r2/(a*a) + 2*r*r2/(a*a*a);
}

//}

//==================== EWA ( reference / 2-tex / 4-tex) ====================
 
/**
*   EWA filter
*   Adapted from an ANSI C implementation from Matt Pharr
*/
vec4 ewaFilter(sampler2D Source, vec2 p0, vec2 du, vec2 dv, int scale){

    vec4 foo = texture(Source,p0);
   
    //don't bother with elliptical filtering if the scale is very small
    if(scale<2)
        return foo;
 
    p0 -=vec2(0.5,0.5)/scale;
    vec2 p = scale * p0;
 
    float ux = FILTER_WIDTH * du.s * scale;
    float vx = FILTER_WIDTH * du.t * scale;
    float uy = FILTER_WIDTH * dv.s * scale;
    float vy = FILTER_WIDTH * dv.t * scale;
 
    // compute ellipse coefficients
    // A*x*x + B*x*y + C*y*y = F.
    float A = vx*vx+vy*vy+1;
    float B = -2*(ux*vx+uy*vy);
    float C = ux*ux+uy*uy+1;
    float F = A*C-B*B/4.;
 
    // Compute the ellipse's (u,v) bounding box in texture space
    float bbox_du = 2. / (-B*B+4.0*C*A) * sqrt((-B*B+4.0*C*A)*C*F);
    float bbox_dv = 2. / (-B*B+4.0*C*A) * sqrt(A*(-B*B+4.0*C*A)*F);
 
    //the ellipse bbox             
    int u0 = int(floor(p.s - bbox_du));
    int u1 = int(ceil (p.s + bbox_du));
    int v0 = int(floor(p.t - bbox_dv));
    int v1 = int(ceil (p.t + bbox_dv));
 
    // Heckbert MS thesis, p. 59; scan over the bounding box of the ellipse
    // and incrementally update the value of Ax^2+Bxy*Cy^2; when this
    // value, q, is less than F, we're inside the ellipse so we filter
    // away..
    vec4 num= vec4(0., 0., 0., 1.);
    float den = 0;
    float ddq = 2 * A;
    float U = u0 - p.s;
   
    for (int v = v0; v <= v1; ++v) {
        float V = v - p.t;
        float dq = A*(2*U+1) + B*V;
        float q = (C*V + B*U)*V + A*U*U;

        for (int u = u0; u <= u1; ++u) {
            if (q < F)
            {
                float r2 = q / F;
                float weight = FILTER_FUNC(r2);
           
                num += weight* texture(Source, vec2(u+0.5,v+0.5)/scale);
                den += weight;
            }
            q += dq;
            dq += ddq;
        }

    }
 
 
    vec4 color = num*(1./den);
    return color;
}

vec4 texture2DEWA(sampler2D tex, vec2 coords){
 
    vec2 du = dFdx(coords);
    vec2 dv = dFdy(coords);
   
    int scale = textureSize(tex, 0).x;
 
    return ewaFilter(tex, coords, du, dv, scale );
 
}

vec2 Warp(vec2 texCoord){
  vec2 curvedCoords = texCoord * 2.0 - 1.0;
  float curvedCoordsDistance = sqrt(curvedCoords.x*curvedCoords.x+curvedCoords.y*curvedCoords.y);

  curvedCoords = curvedCoords / curvedCoordsDistance;

  curvedCoords = curvedCoords * (1.0-pow(vec2(1.0-(curvedCoordsDistance/1.4142135623730950488016887242097)),(1.0/(1.0+Distortion*0.2))));

  curvedCoords = curvedCoords / (1.0-pow(vec2(0.29289321881345247559915563789515),(1.0/(vec2(1.0)+Distortion*0.2))));

  curvedCoords = curvedCoords * 0.5 + 0.5;
  return curvedCoords;
}

// Lottes' public domain mask code
// Shadow mask.
vec3 Mask(vec2 pos){
  vec3 mask=vec3(params.maskDark,params.maskDark,params.maskDark);
  
// Very compressed TV style shadow mask.
  if (params.shadowMask == 1.0) {
    float line=params.maskLight;
    float odd=0.0;
    if(fract(pos.x/6.0)<0.5)odd=1.0;
    if(fract((pos.y+odd)/2.0)<0.5)line=params.maskDark;  
    pos.x=fract(pos.x/3.0);
   
    if(pos.x<0.333)mask.r=params.maskLight;
    else if(pos.x<0.666)mask.g=params.maskLight;
    else mask.b=params.maskLight;
    mask*=line;  
  } 

// Aperture-grille.
  else if (params.shadowMask == 2.0) {
    pos.x=fract(pos.x/3.0);

    if(pos.x<0.333)mask.r=params.maskLight;
    else if(pos.x<0.666)mask.g=params.maskLight;
    else mask.b=params.maskLight;
  } 

// Stretched VGA style shadow mask (same as prior shaders).
  else if (params.shadowMask == 3.0) {
    pos.x+=pos.y*3.0;
    pos.x=fract(pos.x/6.0);

    if(pos.x<0.333)mask.r=params.maskLight;
    else if(pos.x<0.666)mask.g=params.maskLight;
    else mask.b=params.maskLight;
  }

// VGA style shadow mask.
  else if (params.shadowMask == 4.0) {
    pos.xy=floor(pos.xy*vec2(1.0,0.5));
    pos.x+=pos.y*3.0;
    pos.x=fract(pos.x/6.0);

    if(pos.x<0.333)mask.r=params.maskLight;
    else if(pos.x<0.666)mask.g=params.maskLight;
    else mask.b=params.maskLight;
  }

  return mask;
}

// torridgristle's shadowmask code
const float Pi = 3.1415926536;

vec3 SinPhosphor(vec3 image)
{
    float MaskR = sin(params.OutputSize.x*vTexCoord.x*Pi*1.0+Pi*0.00000+vTexCoord.y*params.OutputSize.y*Pi*0.5)*0.5+0.5;
    float MaskG = sin(params.OutputSize.x*vTexCoord.x*Pi*1.0+Pi*1.33333+vTexCoord.y*params.OutputSize.y*Pi*0.5)*0.5+0.5;
    float MaskB = sin(params.OutputSize.x*vTexCoord.x*Pi*1.0+Pi*0.66667+vTexCoord.y*params.OutputSize.y*Pi*0.5)*0.5+0.5;
    
    vec3 Mask = vec3(MaskR,MaskG,MaskB);
    
    Mask = min(Mask*2.0,1.0);
    
    return vec3(Mask * image);
}

// cgwg's aperture grille
vec3 cgwg_mask(vec3 image)
{
   float mask = 1.0 - params.DOTMASK_STRENGTH;
   //Output pixels are alternately tinted green and magenta
   vec3 dotMaskWeights = mix(vec3(1.0, mask, 1.0),
						  vec3(mask, 1.0, mask),
						  floor(mod(mod_factor, 2.0)));
   return image * dotMaskWeights;
}

void main()
{
   FragColor = texture2DEWA(SourceImage, Warp(vTexCoord));
   if(params.shadowMask < -0.5)
      return;
   else
   {
      if (params.shadowMask == 0.0)
      {
         FragColor.rgb = cgwg_mask(FragColor.rgb);
         FragColor = vec4(FragColor.rgb, 1.0);
         return;
      }
      else if (params.shadowMask == 5.0)
      {
         FragColor.rgb *= SinPhosphor(FragColor.rgb);
      }
      else
      {
         // Lottes mask needs linear gamma
         FragColor.rgb = pow(FragColor.rgb, vec3(2.2));
         FragColor.rgb *= Mask(floor(1.000001 * vTexCoord.xy * params.OutputSize.xy + vec2(0.5)));
      }
      FragColor = vec4(pow(FragColor.rgb, vec3(1.0/2.2)), 1.0);
   }
}

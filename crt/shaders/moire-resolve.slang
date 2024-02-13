#version 450

// Moire compensation
// by hunterk
// based on work from Timothy Lottes
// https://www.shadertoy.com/view/4dfyWj
// https://www.shadertoy.com/view/MtSfRK
// license: public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float moire_test;
   float moire_mitigation;
	float warpX;
   float warpY;
   float shadowMask;
   float maskDark;
   float maskLight;
   float THICKNESS;
   float DARKNESS;
   float scanline_toggle;
   float mask_curvature;
} params;

#pragma parameter moire_test "Moire Test" 0.0 0.0 1.0 1.0
#pragma parameter moire_mitigation "Moire Mitigation" 4.0 1.0 10.0 1.0
#pragma parameter warpX "warpX" 0.031 0.0 0.125 0.01
#pragma parameter warpY "warpY" 0.041 0.0 0.125 0.01
#pragma parameter maskDark "maskDark" 0.5 0.0 2.0 0.1
#pragma parameter maskLight "maskLight" 1.5 0.0 2.0 0.1
#pragma parameter shadowMask "shadowMask" 3.0 0.0 4.0 1.0
#pragma parameter THICKNESS "Scanline Thickness" 2.0 1.0 12.0 1.0
#pragma parameter DARKNESS "Scanline Darkness" 0.35 0.0 1.0 0.05
#pragma parameter scanline_toggle "Scanline Toggle" 0.0 0.0 1.0 1.0
#pragma parameter mask_curvature "Mask Curvature" 0.0 0.0 1.0 1.0

#define iTime mod(float(params.FrameCount) / 60.0, 600.0)
#define fragCoord (vTexCoord.xy * params.OutputSize.xy)

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

// Convert from linear to sRGB.
//float Srgb(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}
vec4 Srgb(vec4 c){return pow(c, vec4(1.0 / 2.2));}

// Convert from sRGB to linear.
//float Linear(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}
float Linear(float c){return pow(c, 2.2);}

//
// Semi-Poor Quality Temporal Noise
//

// Base.
// Ripped ad modified from: https://www.shadertoy.com/view/4djSRW
float Noise(vec2 p,float x){p+=x;
 vec3 p3=fract(vec3(p.xyx)*10.1031);
 p3+=dot(p3,p3.yzx+19.19);
 return (fract((p3.x+p3.y)*p3.z)*2.0-1.0) / pow(2.0, 11.0 - params.moire_mitigation);}

// Step 1 in generation of the dither source texture.
float Noise1(vec2 uv,float n){
 float a=1.0,b=2.0,c=-12.0,t=1.0;   
 return (1.0/max(a*4.0+b*4.0,-c))*(
  Noise(uv+vec2(-1.0,-1.0)*t,n)*a+
  Noise(uv+vec2( 0.0,-1.0)*t,n)*b+
  Noise(uv+vec2( 1.0,-1.0)*t,n)*a+
  Noise(uv+vec2(-1.0, 0.0)*t,n)*b+
  Noise(uv+vec2( 0.0, 0.0)*t,n)*c+
  Noise(uv+vec2( 1.0, 0.0)*t,n)*b+
  Noise(uv+vec2(-1.0, 1.0)*t,n)*a+
  Noise(uv+vec2( 0.0, 1.0)*t,n)*b+
  Noise(uv+vec2( 1.0, 1.0)*t,n)*a+
 0.0);}
    
// Step 2 in generation of the dither source texture.
float Noise2(vec2 uv,float n){
 float a=1.0,b=2.0,c=-2.0,t=1.0;   
 return (1.0/(a*4.0+b*4.0))*(
  Noise1(uv+vec2(-1.0,-1.0)*t,n)*a+
  Noise1(uv+vec2( 0.0,-1.0)*t,n)*b+
  Noise1(uv+vec2( 1.0,-1.0)*t,n)*a+
  Noise1(uv+vec2(-1.0, 0.0)*t,n)*b+
  Noise1(uv+vec2( 0.0, 0.0)*t,n)*c+
  Noise1(uv+vec2( 1.0, 0.0)*t,n)*b+
  Noise1(uv+vec2(-1.0, 1.0)*t,n)*a+
  Noise1(uv+vec2( 0.0, 1.0)*t,n)*b+
  Noise1(uv+vec2( 1.0, 1.0)*t,n)*a+
 0.0);}

// Compute temporal dither from integer pixel position uv.
float Noise3(vec2 uv){return Noise2(uv,fract(iTime));}    

// Energy preserving dither, for {int pixel pos,color,amount}.
vec2 Noise4(vec2 uv,vec2 c,float a){
 // Grain value {-1 to 1}.
 vec2 g=vec2(Noise3(uv)*2.0);
 // Step size for black in non-linear space.
 float rcpStep=1.0/(256.0-1.0);
 // Estimate amount negative which still quantizes to zero.
 vec2 black=vec2(0.5*Linear(rcpStep));
 // Estimate amount above 1.0 which still quantizes to 1.0.
 vec2 white=vec2(2.0-Linear(1.0-rcpStep));
 // Add grain.
 return vec2(clamp(c+g*min(c+black,min(white-c,a)),0.0,1.0));}

//
// Pattern
//

// 4xMSAA pattern for quad given integer coordinates.
//
//  . x . . | < pixel
//  . . . x |
//  x . . .
//  . . x .
//
//  01
//  23
//
vec2 Quad4(vec2 pp){
 int q=(int(pp.x)&1)+((int(pp.y)&1)<<1);
 if(q==0)return pp+vec2( 0.25,-0.25);
 if(q==1)return pp+vec2( 0.25, 0.25);
 if(q==2)return pp+vec2(-0.25,-0.25);
         return pp+vec2(-0.25, 0.25);}

// Rotate {0.0,r} by a {-1.0 to 1.0}. 
vec2 Rot(float r,float a){return vec2(r*cos(a*3.14159),r*sin(a*3.14159));}

//
// POOR QUALITY JITTERED
//

// Jittered position.
vec2 Jit(vec2 pp){
 // Start with better baseline pattern.
 pp=Quad4(pp);
 // Very poor quality (clumping) move in disc around pixel.
 float n=Noise(pp,fract(iTime));    
 float m=Noise(pp,fract(iTime*0.333))*0.5+0.5;
 m = sqrt(m) / 4.0;
 return pp+Rot(0.707*0.5*m,n);}

//
// POOR QUALITY JITTERED 4x
//

// Gaussian filtered jittered tap.
void JitGaus4(inout vec2 sumC,inout vec2 sumW,vec2 pp,vec2 mm){
 vec2 jj=Jit(pp);
 vec2 c=jj;
 vec2 vv=mm-jj;
 float w=exp2(-1.0*dot(vv,vv));    
 sumC+=c*vec2(w); sumW+=vec2(w);}   

// Many tap gaussian from poor quality jittered 4/sample per pixel
//
//  . x x x . 
//  x x x x x 
//  x x x x x 
//  x x x x x 
//  . x x x . 
//
vec2 ResolveJitGaus4(vec2 pp){
 vec2 ppp=(pp);
 vec2 sumC=vec2(0.0);
 vec2 sumW=vec2(0.0);
 JitGaus4(sumC,sumW,ppp+vec2(-1.0,-2.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 0.0,-2.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 1.0,-2.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2(-2.0,-1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2(-1.0,-1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 0.0,-1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 1.0,-1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 2.0,-1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2(-2.0, 0.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2(-1.0, 0.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 0.0, 0.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 1.0, 0.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 2.0, 0.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2(-2.0, 1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2(-1.0, 1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 0.0, 1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 1.0, 1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 2.0, 1.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2(-1.0, 2.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 0.0, 2.0),pp);
 JitGaus4(sumC,sumW,ppp+vec2( 1.0, 2.0),pp);
 return sumC/sumW;}

vec2 moire_resolve(vec2 coord){
   vec2 pp = coord;
   vec2 cc = vec2(0.0, 0.0);
   
   cc = ResolveJitGaus4(pp);
   cc = Noise4(pp, cc, 1.0 / 32.0);
   cc = (params.moire_test > 0.5) ? pp : cc + vec2(0.0105, 0.015);
   
   return cc;
}

// Distortion of scanlines, and end of screen alpha.
vec2 Warp(vec2 pos)
{
    pos  = pos*2.0-1.0;    
    pos *= vec2(1.0 + (pos.y*pos.y)*params.warpX, 1.0 + (pos.x*pos.x)*params.warpY);
    
    return pos*0.5 + 0.5;
}

// Shadow mask.
vec3 Mask(vec2 pos)
{
    vec3 mask = vec3(params.maskDark, params.maskDark, params.maskDark);
  
    // Very compressed TV style shadow mask.
    if (params.shadowMask == 1.0) 
    {
        float line = params.maskLight;
        float odd = 0.0;
        
        if (fract(pos.x*0.166666666) < 0.5) odd = 1.0;
        if (fract((pos.y + odd) * 0.5) < 0.5) line = params.maskDark;  
        
        pos.x = fract(pos.x*0.333333333);

        if      (pos.x < 0.333) mask.r = params.maskLight;
        else if (pos.x < 0.666) mask.g = params.maskLight;
        else                    mask.b = params.maskLight;
        mask*=line;  
    } 

    // Aperture-grille.
    else if (params.shadowMask == 2.0) 
    {
        pos.x = fract(pos.x*0.333333333);

        if      (pos.x < 0.333) mask.r = params.maskLight;
        else if (pos.x < 0.666) mask.g = params.maskLight;
        else                    mask.b = params.maskLight;
    } 

    // Stretched VGA style shadow mask (same as prior shaders).
    else if (params.shadowMask == 3.0) 
    {
        pos.x += pos.y*3.0;
        pos.x  = fract(pos.x*0.166666666);

        if      (pos.x < 0.333) mask.r = params.maskLight;
        else if (pos.x < 0.666) mask.g = params.maskLight;
        else                    mask.b = params.maskLight;
    }

    // VGA style shadow mask.
    else if (params.shadowMask == 4.0) 
    {
        pos.xy  = floor(pos.xy*vec2(1.0, 0.5));
        pos.x  += pos.y*3.0;
        pos.x   = fract(pos.x*0.166666666);

        if      (pos.x < 0.333) mask.r = params.maskLight;
        else if (pos.x < 0.666) mask.g = params.maskLight;
        else                    mask.b = params.maskLight;
    }

    return mask;
}

vec4 scanlines(vec4 frame, vec2 coord, vec2 texture_size, vec2
	video_size, vec2 output_size)
{
	float lines = fract(coord.y * texture_size.y);
	float scale_factor = floor((output_size.y / video_size.y) + 0.4999);
    float lightness = 1.0 - params.DARKNESS;
	return (params.scanline_toggle > 0.5 && (lines < (1.0 / scale_factor * params.THICKNESS)))
		? frame * vec4(lightness, lightness, lightness, lightness) : frame;
}

void main(){
 vec2 pp = moire_resolve(vTexCoord.xy);
 vec2 cc = Warp(pp);
 
 // Output
 FragColor=pow(texture(Source, cc), vec4(2.2));
 FragColor = scanlines(FragColor, cc, params.SourceSize.xy, params.SourceSize.xy, params.OutputSize.xy);
     if (params.shadowMask > 0.0)
        FragColor.rgb *= Mask(((params.mask_curvature > 0.5) ? vTexCoord.xy : cc) * params.OutputSize.xy * 1.000001);
 FragColor = Srgb(FragColor);
     /* TODO/FIXME - hacky clamp fix */
    vec2 bordertest = (cc);
    if ( bordertest.x > 0.0106 && bordertest.x < 0.9999 && bordertest.y > 0.016 && bordertest.y < 0.9999)
        FragColor.rgb = FragColor.rgb;
    else
        FragColor.rgb = vec3(0.0);
}

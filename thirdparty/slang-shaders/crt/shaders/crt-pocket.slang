#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float gammain,gammaout,warpX,warpY,vignette,vign,corners,beam1,beam2,
    scanline1,scanline2,interlace,progress,boost1,boost2,sat,flick;
    } params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    float DOTMASK_STRENGTH,maskDark,maskLight,shadowMask,bgr,maskl,maskh,gl,
    ntsc,TEMP,size;
} global;

#pragma parameter warpX "Curvature X" 0.03 0.00 0.20 0.01 
#define warpX params.warpX
#pragma parameter warpY "Curvature Y" 0.05 0.00 0.20 0.01
#define warpY params.warpY
#pragma parameter corners "Corners cut On/Off" 1.0 0.0 1.0 1.0
#define corners params.corners
#pragma parameter vignette "Vignette On/Off" 1.0 0.0 1.0 1.0
#define vignette params.vignette
#pragma parameter vign "Vignette Strength" 0.10 0.0 0.50 0.01
#define vign params.vign
#pragma parameter shadowMask "Mask Style 0:cgwg,1-4:Lottes,5:Sin" 1.0 0.0 5.0 1.0
#define shadowMask global.shadowMask
#pragma parameter size "Mask Size" 1.0 1.0 2.0 1.0
#define size global.size
#pragma parameter maskl "Mask Low" 0.9 0.0 1.0 0.05
#define maskl global.maskl
#pragma parameter maskh "Mask High" 0.6 0.0 1.0 0.05
#define maskh global.maskh
#pragma parameter bgr "RGB/BGR subpixels" 1.0 0.0 1.0 1.0
#define bgr global.bgr
#pragma parameter DOTMASK_STRENGTH "CGWG Dot Mask Strength" 0.7 0.0 1.0 0.05
#define DOTMASK_STRENGTH global.DOTMASK_STRENGTH
#pragma parameter maskDark "Lottes maskDark" 0.3 0.0 2.0 0.1
#define maskDark global.maskDark
#pragma parameter maskLight "Lottes maskLight" 1.5 0.0 2.0 0.1
#define maskLight global.maskLight
#pragma parameter interlace "Interlace On/Off" 1.0 0.0 1.0 1.0
#define interlace params.interlace
#pragma parameter progress "Force 240p" 0.0 0.0 1.0 1.0  
#define progress params.progress
#pragma parameter flick "Flicker Strength" 4.0 0.0 10.0 0.1
#define flick params.flick
#pragma parameter beam1 "Scanline Beam Low" 6.0 1.0 15.0 0.5
#define beam1 params.beam1
#pragma parameter beam2 "Scanline Beam High" 8.0 1.0 15.0 0.5
#define beam2 params.beam2
#pragma parameter scanline1 "Scanline Low" 1.35 0.5 3.0 0.05
#define scanline1 params.scanline1
#pragma parameter scanline2 "Scanline High" 1.05 0.5 3.0 0.05
#define scanline2 params.scanline2
#pragma parameter ntsc "NTSC Colors" 1.0 0.0 1.0 1.0
#define ntsc global.ntsc
#pragma parameter TEMP "Color Temperature in Kelvins"  8591.0 1031.0 12047.0 72.0
#define TEMP global.TEMP
#pragma parameter gammain "Gamma In" 2.45 1.0 4.0 0.05
#define gammain params.gammain
#pragma parameter gammaout "Gamma Out" 2.25 1.0 4.0 0.05
#define gammaout params.gammaout
#pragma parameter sat "Saturation" 1.0 0.0 2.0 0.01
#define sat params.sat
#pragma parameter boost1 "Boost Dark Colors" 1.45 1.0 2.0 0.05
#define boost1 params.boost1
#pragma parameter boost2 "Boost Bright Colors" 1.25 1.0 2.0 0.05
#define boost2 params.boost2
#pragma parameter gl "Glow Strength" 0.15 0.0 1.0 0.01
#define gl global.gl


#define pi 3.141592
#define SourceSize params.SourceSize
#define OutputSize params.OutputSize
#define FrameCount params.FrameCount
#define OriginalSize params.OriginalSize
#define timer float(FrameCount*2.0)
#define flicker sin(timer)*flick/1000.0
#define mod_factor vTexCoord.x * SourceSize.x * OutputSize.x/size / SourceSize.x




#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord*1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;



vec3 Mask(vec2 pos){
  vec3 mask=vec3(maskDark,maskDark,maskDark);
 if (shadowMask == 0.0)
{
   float mask = 1.0 - DOTMASK_STRENGTH;
   //Output pixels are alternately tinted green and magenta
   vec3 dotMaskWeights = mix(vec3(1.0, mask, 1.0),
                          vec3(mask, 1.0, mask),
                          floor(mod(mod_factor, 2.0)));
   return dotMaskWeights;
}

// Very compressed TV style shadow mask.
    else if (shadowMask == 1.0) {
    float line=maskLight;
    float odd=0.0;
    if(fract(pos.x/6.0)<0.5)odd=1.0;
    if(fract((pos.y+odd)/2.0)<0.5)line=maskDark;  
    pos.x=fract(pos.x/3.0);
   
    if(pos.x<0.333)     bgr == 1.0? mask.b=maskLight : mask.r=maskLight;
    else if(pos.x<0.666)mask.g=maskLight;
    else                bgr == 1.0? mask.r=maskLight : mask.b=maskLight;
    mask*=line;  
  } 

// Aperture-grille.
  else if (shadowMask == 2.0) {
    pos.x=fract(pos.x/3.0);

    if(pos.x<0.333)     bgr == 1.0? mask.b=maskLight : mask.r=maskLight;
    else if(pos.x<0.666)mask.g=maskLight;
    else                bgr == 1.0? mask.r=maskLight : mask.b=maskLight;
  } 

// Stretched VGA style shadow mask (same as prior shaders).
  else if (shadowMask == 3.0) {
    pos.x+=pos.y*3.0;
    pos.x=fract(pos.x/6.0);

    if(pos.x<0.333)     bgr == 1.0? mask.b=maskLight : mask.r=maskLight;
    else if(pos.x<0.666)mask.g=maskLight;
    else                bgr == 1.0? mask.r=maskLight : mask.b=maskLight;
  }

// VGA style shadow mask.
  else if (shadowMask == 4.0) {
    pos.xy=floor(pos.xy*vec2(1.0,0.5));
    pos.x+=pos.y*3.0;
    pos.x=fract(pos.x/6.0);

    if(pos.x<0.333)     bgr == 1.0? mask.b=maskLight : mask.r=maskLight;
    else if(pos.x<0.666)mask.g=maskLight;
    else                bgr == 1.0? mask.r=maskLight : mask.b=maskLight;
  }

  else if (shadowMask == 5.0)
  {
    float MaskR = sin(OutputSize.x*vTexCoord.x*pi*1.0+pi*0.00000+vTexCoord.y*OutputSize.y*pi*0.5)*0.5+0.5;
    float MaskG = sin(OutputSize.x*vTexCoord.x*pi*1.0+pi*1.33333+vTexCoord.y*OutputSize.y*pi*0.5)*0.5+0.5;
    float MaskB = sin(OutputSize.x*vTexCoord.x*pi*1.0+pi*0.66667+vTexCoord.y*OutputSize.y*pi*0.5)*0.5+0.5;
    
    vec3 Mask; bgr == 1.0? Mask = vec3(MaskB,MaskG,MaskR) : Mask = vec3(MaskR,MaskG,MaskB) ;
    
    Mask = min(Mask*2.0,1.0);
    
    return vec3(Mask);
}

  return mask;
}



//Lanczos filter
float sinc(float x)
{
    return sin(x * pi) / (x * pi); 
}

float lanczosWeight(float d, float n)
{
    return (d == 0.0) ? (1.0) : (d*d < n*n ? sinc(d) * sinc(d / n) : 0.0);
}

vec3 Lanczos3(vec2 uv, vec2 InvResolution)
{
    vec2 center = uv - (mod(uv / InvResolution, 1.0)-0.5) * InvResolution;// texel center
    vec2 offset = (uv - center)/InvResolution;// relevant texel position in the range -0.5～+0.5
    
    vec3 col = vec3(0.0);
    float weight = 0.0;
 //loop   
    for(int x = -2; x < 3; x++)
    {
        for(int y = -1; y < 2; y++)
        {
        
            float wx = lanczosWeight(float(x)-offset.x, 2.0);
            float wy = lanczosWeight(float(y)-offset.y, 1.0);
            float w = wx * wy;
        
        col +=  w * texture(Source, center + vec2(x,y) * InvResolution).rgb;
        weight += w;
        }
    }
    col /= weight;
    
    return col;
}

// Distortion of scanlines
vec2 Warp(vec2 pos)
{
    pos  = pos*2.0-1.0;    
    pos *= vec2(1.0 + (pos.y*pos.y)*warpX, 1.0 + (pos.x*pos.x)*warpY);
    return pos*0.5 + 0.5;
}

//Vignette
mat3 vignet()
{
    vec2 vpos = vTexCoord;

    vpos *= 1.0 - vpos.xy;
    float vig = vpos.x * vpos.y * 50.0;
    vig = min(pow(vig, vign), 1.0); 
    if (vignette == 0.0) vig=1.0;
   
    return mat3(vig, 0, 0,
                 0,   vig, 0,
                 0,    0, vig);
}

float sw(float y, float l)
{
    float scan = mix(beam1, beam2, y);
    float tmp = mix(scanline1, scanline2, l);
    float ex = y*tmp;
    return exp2(-scan*ex*ex);
}


vec3 glow(vec2 pos)
{
    vec3 col = vec3(0.0);
    float x = SourceSize.z;
    float y = SourceSize.w;
    float w1 = 0.15;
    float w2 = 0.05;
    col += w1*texture(Source,pos+vec2(x,0.0)).rgb;
    col += w1*texture(Source,pos+vec2(x,y)).rgb;
    col += w1*texture(Source,pos+vec2(x,-y)).rgb;
    col += w1*texture(Source,pos+vec2(-x,0.0)).rgb;
    col += w1*texture(Source,pos+vec2(-x,y)).rgb;
    col += w1*texture(Source,pos+vec2(-x,-y)).rgb;
    col += w1*texture(Source,pos+vec2(0.0,y)).rgb;
    col += w1*texture(Source,pos+vec2(0.0,-y)).rgb;

    col += w2*texture(Source,pos+vec2(2.0*x,0.0)).rgb;
    col += w2*texture(Source,pos+vec2(2.0*x,y)).rgb;
    col += w2*texture(Source,pos+vec2(2.0*x,-y)).rgb;
    col += w2*texture(Source,pos+vec2(-2.0*x,0.0)).rgb;
    col += w2*texture(Source,pos+vec2(-2.0*x,y)).rgb;
    col += w2*texture(Source,pos+vec2(-2.0*x,-y)).rgb;
    col += w1*texture(Source,pos+vec2(0.0,2.0*y)).rgb;
    col += w1*texture(Source,pos+vec2(0.0,-2.0*y)).rgb;
    return col*gl;
}

vec3 DecodeGamma(vec3 color, float gamma)
{
    color = clamp(color, 0.0, 1.0);
    color.r = (color.r <= 0.00313066844250063) ?
    color.r * 12.92 : 1.055 * pow(color.r, 1.0 / gamma) - 0.055;
    color.g = (color.g <= 0.00313066844250063) ?
    color.g * 12.92 : 1.055 * pow(color.g, 1.0 / gamma) - 0.055;
    color.b = (color.b <= 0.00313066844250063) ?
    color.b * 12.92 : 1.055 * pow(color.b, 1.0 / gamma) - 0.055;

    return color;
}




vec3 RGBtoXYZ(vec3 RGB)
{
    const mat3x3 m = mat3x3(
        0.6068909, 0.1735011, 0.2003480,
        0.2989164, 0.5865990, 0.1144845,
        0.0000000, 0.0660957, 1.1162243);
    return RGB * m;
}

 vec3 NTSC(vec3 c)
 {
     vec3 v = vec3(pow(c.r, 2.2), pow(c.g, 2.2), pow(c.b, 2.2)); //Inverse Companding
     return RGBtoXYZ(v);
 }

vec3 XYZtoSRGB(vec3 XYZ)
{
    const mat3x3 m = mat3x3(
        3.2404542,-1.5371385,-0.4985314,
       -0.9692660, 1.8760108, 0.0415560,
        0.0556434,-0.2040259, 1.0572252);
    return XYZ * m;
}

vec3 sRGB(vec3 c)
 {
     vec3 v = XYZtoSRGB(c);
     v = DecodeGamma(v, 2.4); //Companding
 
     return v;
 }


// NTSC RGB to sRGB
vec3 NTSCtoSRGB( vec3 c )
 { 
     return sRGB(NTSC( c )); 
 }

float saturate(float v) 
    { 
        return clamp(v, 0.0, 1.0);       
    }

vec3 ColorTemp(float temperatureInKelvins)
{
    vec3 retColor;
    temperatureInKelvins = clamp(temperatureInKelvins, 1000.0, 40000.0) / 100.0;
    
    if (temperatureInKelvins <= 66.0)
    {
        retColor.r = 1.0;
        retColor.g = saturate(0.39008157876901960784 * log(temperatureInKelvins) - 0.63184144378862745098);
    }
    else
    {
        float t = temperatureInKelvins - 60.0;
        retColor.r = saturate(1.29293618606274509804 * pow(t, -0.1332047592));
        retColor.g = saturate(1.12989086089529411765 * pow(t, -0.0755148492));
    }
    
    if (temperatureInKelvins >= 66.0)
        retColor.b = 1.0;
    else if(temperatureInKelvins <= 19.0)
        retColor.b = 0.0;
    else
        retColor.b = saturate(0.54320678911019607843 * log(temperatureInKelvins - 10.0) - 1.19625408914);

    return retColor;
}

void main()
{
    vec2 pos = Warp(vTexCoord);
    vec2 uv = pos * vec2(0.5, 2.0);
    vec2 InvResolution = SourceSize.zw;

    float f = fract(0.5+pos.y*SourceSize.y);
    float f2 = fract(pos.y*SourceSize.y);

//handle interlacing
if (OriginalSize.y > 400.0 && interlace == 1.0) 
    {
    f = fract(0.5+pos.y*SourceSize.y/2.0);
    f2 = fract((pos.y-0.5)*SourceSize.y/2.0);
    sin(timer) > 0.0 ? f=f : f=f2;
    }
if (OriginalSize.y > 400.0 && interlace == 0.0 && progress == 1.0)
    {
    f = fract(0.5+pos.y*SourceSize.y/2.0);
    }

    
    vec3 res = Lanczos3(pos,InvResolution);
    if (ntsc == 1.0) res = NTSCtoSRGB(res);
    res *= ColorTemp(TEMP);

    float lumscan = max(max(res.r,res.g),res.b);
    float lumsat  = dot(vec3(0.22,0.7,0.08),res);
    res = pow(res,vec3(gammain));
    res = res*sw(f,lumscan) + res*sw(abs(1.0-f),lumscan);
    float msk = mix(maskl,maskh,lumscan);
    res *= mix(vec3(1.0),Mask(floor(vTexCoord.xy * OutputSize.xy/size + vec2(0.5))),msk);

    res *= mix(boost1,boost2,lumsat);
    res = pow(res,vec3(1.0/gammaout));
    res *= vignet();
    res += flicker;

    vec2 corn = min(pos,vec2(1.0)-pos); // This is used to mask corners
    corn.x = 0.0001/corn.x;             
    res +=glow(pos);
    if (corners == 1.0 && corn.y <= corn.x || corn.x < 0.0001 ) res = vec3(0.0);
    res = mix(vec3(lumsat),res,sat);

    FragColor = vec4(res,1.0);
}

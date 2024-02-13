// Colorspace Tools
// ported from Asmodean's PsxFX Shader Suite v2.00
// NTSC color code from SimoneT
// Jzazbz code from torridgristle
// License: GPL v2+

/*------------------------------------------------------------------------------
                       [GAMMA CORRECTION CODE SECTION]
------------------------------------------------------------------------------*/

vec3 EncodeGamma(vec3 color, float gamma)
{
    color = clamp(color, 0.0, 1.0);
    color.r = (color.r <= 0.0404482362771082) ?
    color.r / 12.92 : pow((color.r + 0.055) / 1.055, gamma);
    color.g = (color.g <= 0.0404482362771082) ?
    color.g / 12.92 : pow((color.g + 0.055) / 1.055, gamma);
    color.b = (color.b <= 0.0404482362771082) ?
    color.b / 12.92 : pow((color.b + 0.055) / 1.055, gamma);

    return color;
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

#ifdef GAMMA_CORRECTION
vec4 GammaPass(vec4 color, vec2 texcoord)
{
    const float GammaConst = 2.233333;
    color.rgb = EncodeGamma(color.rgb, GammaConst);
    color.rgb = DecodeGamma(color.rgb, float(Gamma));

    return color;
}
#endif

// more gamma linearization algos
vec3 linear_srgb(vec3 x) {
#ifdef GAMMA_CORRECT
// use slower, more accurate calculation
    return mix(1.055*pow(x, vec3(1./2.4)) - 0.055, 12.92*x, step(x,vec3(0.0031308)));
#else
// use faster, less accurate calculation
    return pow(x,vec3(1.0/2.2));
#endif
}
 
vec3 srgb_linear(vec3 x) {
#ifdef GAMMA_CORRECT
    return mix(pow((x + 0.055)/1.055,vec3(2.4)), x / 12.92, step(x,vec3(0.04045)));
#else
    return pow(x,vec3(2.2));
#endif
}

vec3 linear_to_sRGB(vec3 color, float gamma)
{
    color = clamp(color, 0.0, 1.0);
    color.r = (color.r <= 0.00313066844250063) ?
    color.r * 12.92 : 1.055 * pow(color.r, 1.0 / gamma) - 0.055;
    color.g = (color.g <= 0.00313066844250063) ?
    color.g * 12.92 : 1.055 * pow(color.g, 1.0 / gamma) - 0.055;
    color.b = (color.b <= 0.00313066844250063) ?
    color.b * 12.92 : 1.055 * pow(color.b, 1.0 / gamma) - 0.055;

    return color.rgb;
}

vec3 sRGB_to_linear(vec3 color, float gamma)
{
    color = clamp(color, 0.0, 1.0);
    color.r = (color.r <= 0.04045) ?
    color.r / 12.92 : pow((color.r + 0.055) / (1.055), gamma);
    color.g = (color.g <= 0.04045) ?
    color.g / 12.92 : pow((color.g + 0.055) / (1.055), gamma);
    color.b = (color.b <= 0.04045) ?
    color.b / 12.92 : pow((color.b + 0.055) / (1.055), gamma);

    return color.rgb;
}

/*------------------------------------------------------------------------------
                       [RGB TO GRAYSCALE / LUMA CODE SECTION]
------------------------------------------------------------------------------*/

// if you're already in linear gamma, definitely use this one ( Y = 0.2126R + 0.7152G + 0.0722B )
// the Rec. 709 spec uses these same coefficients but with gamma-compressed components ( Y' = 0.2126R' + 0.7152G' + 0.0722B' )
float luma(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// for digital formats following CCIR 601 (that is, most digital standard def formats)
// expects gamma-compressed components and doesn't look very good
// ( Y' = 0.299R' + 0.587G' + 0.114B' )
float luma_CCIR601(vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

// SMPTE 240M; used by some transitional 1035i HDTV signals. Expects gamma-compressed components
// ( Y' = 0.212R' + 0.701G' + 0.087B' )
float luma_240M(vec3 color)
{
    return dot(color, vec3(0.212, 0.701, 0.087));
}

// Same as Rec. 709 but with quick-and-dirty gamma linearization added on top
float luma_gamma(vec3 color)
{
    color = color * color;
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return sqrt(luma);
}

/*------------------------------------------------------------------------------
                       [COLORSPACE CONVERSION CODE SECTION]
------------------------------------------------------------------------------*/

/* XYZ color space is a device-invariant representation that encompasses all color sensations that are visible to a person
with average eyesight. Y is the luminance, Z is quasi-equal to blue and X is a mix of the three CIE RGB curves chosen to be non-negative */

vec3 RGBtoXYZ(vec3 RGB)
{
    const mat3x3 m = mat3x3(
        0.6068909, 0.1735011, 0.2003480,
        0.2989164, 0.5865990, 0.1144845,
        0.0000000, 0.0660957, 1.1162243);
    return RGB * m;
}
  
vec3 XYZtoRGB(vec3 XYZ)
{
    const mat3x3 m = mat3x3(
        1.9099961, -0.5324542, -0.2882091,
        -0.9846663,  1.9991710, -0.0283082,
        0.0583056, -0.1183781,  0.8975535); 
    return XYZ * m;
}

vec3 XYZtoSRGB(vec3 XYZ)
{
    const mat3x3 m = mat3x3(
        3.2404542,-1.5371385,-0.4985314,
        -0.9692660, 1.8760108, 0.0415560,
        0.0556434,-0.2040259, 1.0572252);
    return XYZ * m;
}

/* YUV is a color space that takes human perception into account, allowing reduced bandwidth for chrominance components,
as compared with a direct RGB representation. It includes a luminance component, Y, with nonlinear perceptual brightness,
and two color components, U and V. This colorspace was used in the PAL color broadcast standard and is the counterpart to
NTSC's YIQ colorspace. It is still commonly used to describe YCbCr signals. */
  
vec3 RGBtoYUV(vec3 RGB)
{
    const mat3x3 m = mat3x3(
        0.2126, 0.7152, 0.0722,
        -0.09991,-0.33609, 0.436,
        0.615, -0.55861, -0.05639);
    return RGB * m;
}
 
vec3 YUVtoRGB(vec3 YUV)
{
    const mat3x3 m = mat3x3(
        1.000, 0.000, 1.28033,
        1.000,-0.21482,-0.38059,
        1.000, 2.12798, 0.000);
    return YUV * m;
}

/* YIQ is the color space used for analog NTSC color broadcasts, whereby Y stands for luma, I stands for in-phase and 
Q stands for quadrature, referring to the components used in quadrature amplitude modulation. The IQ axes exist on the
same plane as the UV axes from the YUV color space, just rotated 33 degrees. */

vec3 RGBtoYIQ(vec3 RGB)
{
    const mat3x3 m = mat3x3(
        0.2989, 0.5870, 0.1140,
        0.5959, -0.2744, -0.3216,
        0.2115, -0.5229, 0.3114);
    return RGB * m;
}

vec3 YIQtoRGB(vec3 YIQ)
{
    const mat3x3 m = mat3x3(
        1.0, 0.956, 0.6210,
        1.0, -0.2720, -0.6474,
        1.0, -1.1060, 1.7046);
    return YIQ * m;
}
  
vec3 XYZtoYxy(vec3 XYZ)
{
    float w = (XYZ.r + XYZ.g + XYZ.b);
    vec3 Yxy;
    Yxy.r = XYZ.g;
    Yxy.g = XYZ.r / w;
    Yxy.b = XYZ.g / w;
  
    return Yxy;
}
  
vec3 YxytoXYZ(vec3 Yxy)
{
    vec3 XYZ;
    XYZ.g = Yxy.r;
    XYZ.r = Yxy.r * Yxy.g / Yxy.b;
    XYZ.b = Yxy.r * (1.0 - Yxy.g - Yxy.b) / Yxy.b;
  
    return XYZ;
}

/* CMYK--aka process color or four color--is a subtractive color model based on the CMY color model that is
used in color printing and to describe the printing process itself. C is for Cyan, M is for Magenta, Y is for
Yellow and K is for 'key' or black. */

// RGB <-> CMYK conversions require 4 channels
vec4 RGBtoCMYK(vec3 RGB){
    float Red     = RGB.r;
    float Green   = RGB.g;
    float Blue    = RGB.b;
    float Black   = min(1.0 - Red, min(1.0 - Green, 1.0 - Blue));
    float Cyan    =    (1.0 - Red   - Black) / (1.0 - Black);
    float Magenta =    (1.0 - Green - Black) / (1.0 - Black);
    float Yellow  =    (1.0 - Blue  - Black) / (1.0 - Black);
    return vec4(Cyan, Magenta, Yellow, Black);
}
 
vec3 CMYKtoRGB(vec4 CMYK){
    float Cyan    = CMYK.x;
    float Magenta = CMYK.y;
    float Yellow  = CMYK.z;
    float Black   = CMYK.w;
    float Red     = 1.0 - min(1.0, Cyan    * (1.0 - Black) + Black);
    float Green   = 1.0 - min(1.0, Magenta * (1.0 - Black) + Black);
    float Blue    = 1.0 - min(1.0, Yellow  * (1.0 - Black) + Black);
    return vec3(Red, Green, Blue);
}
  
// Converting pure hue to RGB
vec3 HUEtoRGB(float H)
{
    float R = abs(H * 6.0 - 3.0) - 1.0;
    float G = 2.0 - abs(H * 6.0 - 2.0);
    float B = 2.0 - abs(H * 6.0 - 4.0);

    return clamp(vec3(R, G, B), 0.0, 1.0);
}

// Converting RGB to hue/chroma/value
vec3 RGBtoHCV(vec3 RGB)
{
    vec4 BG = vec4(RGB.bg,-1.0, 2.0 / 3.0);
    vec4 GB = vec4(RGB.gb, 0.0,-1.0 / 3.0);

    vec4 P = (RGB.g < RGB.b) ? BG : GB;

    vec4 XY = vec4(P.xyw, RGB.r);
    vec4 YZ = vec4(RGB.r, P.yzx);

    vec4 Q = (RGB.r < P.x) ? XY : YZ;

    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6.0 * C + 1e-10) + Q.z);

    return vec3(H, C, Q.x);
}
  
vec3 RGBtoHSV(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
    vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 HSVtoRGB(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// conversion from NTSC RGB Reference White D65 ( color space used by NA/Japan TV's ) to XYZ
vec3 NTSC(vec3 c)
 {
     vec3 v = vec3(pow(c.r, 2.2), pow(c.g, 2.2), pow(c.b, 2.2)); //Inverse Companding
     return RGBtoXYZ(v);
 }
 
// conversion from XYZ to sRGB Reference White D65 ( color space used by windows ) 
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

const float Pi = 3.1415926535897932384626433832795;
 
//  ---  Reference White Values  ---  //{
       
const vec3 D50 = vec3(0.9642, 1.0000, 0.8251);
const vec3 D55 = vec3(0.9568, 1.0000, 0.9214);
const vec3 D65 = vec3(0.9504, 1.0000, 1.0888);
    //D9000 apparently isn't a real standard so here's the CCT daylight calculation result
const vec3 D9000 = vec3(0.9520, 1.0000, 1.3661);
    //D9300 apparently isn't a real standard so here's the CCT daylight calculation result
const vec3 D9300 = vec3(0.95271,1.00000,1.39177);
        //Various CRT monitors, Duv describes distance from the blackbody curve. The smaller it is, the closer to "white" it is. +/- 0.006 is recommended by ANSI and EnergyStar.
    //NEC Multisync C400, claims 9300K but it isn't
const vec3 D9000NEC = vec3(0.88889,1.00000,1.28571);//8890K, 0.0139 Duv
    //KDS VS19
const vec3 D9000KDS = vec3(0.90354,1.00000,1.31190);//8939K, 0.0114 Duv
//}
 
//  ---  sRGB  ---  //
vec3 XYZ_to_sRGB(vec3 x)
{
    x = x * mat3x3( 3.2404542, -1.5371385, -0.4985314, -0.9692660, 1.8760108, 0.0415560, 0.0556434, -0.2040259, 1.0572252 );
    x = mix(1.055*pow(x, vec3(1./2.4)) - 0.055, 12.92*x, step(x,vec3(0.0031308)));
    return x;
}
 
vec3 sRGB_to_XYZ(vec3 x)
{
    x = mix(pow((x + 0.055)/1.055,vec3(2.4)), x / 12.92, step(x,vec3(0.04045)));
    x = x * mat3x3( 0.4124564, 0.3575761, 0.1804375, 0.2126729, 0.7151522, 0.0721750, 0.0193339, 0.1191920, 0.9503041 );
    return x;
}
 
/* Jzazbz is a color space designed for perceptual uniformity in high dynamic range (HDR) and wide color gamut (WCG) applications.
It is conceptually similar to CIE Lab but is considered more "modern". As compared with Lab, perceptual color differences are
predicted by Euclidean distance, it is more perceptually uniform and changes in saturation or lightness produce less shifts in hue
(i.e., increased hue linearity). Jzazbz and JzCzhz are used by ImageMagick and not much else. */

vec3 XYZ_to_Jzazbz(vec3 XYZ)
{
    float b = 1.15;
    float g = 0.66;
    vec3 XYZprime = XYZ;
    XYZprime.x = XYZ.x * b - (b - 1) * XYZ.z;
    XYZprime.y = XYZ.y * g - (g - 1) * XYZ.x;
    XYZprime.z = XYZ.z;
    vec3 LMS = XYZprime * mat3x3(0.41478972, 0.579999, 0.0146480, -0.2015100, 1.120649, 0.0531008, -0.0166008, 0.264800, 0.6684799);
    float c1 = 3424 / pow(2.0,12.0);
    float c2 = 2413 / pow(2.0,7.0);
    float c3 = 2392 / pow(2.0,7.0);
    float n  = 2610 / pow(2.0,14.0);
    float p  = 1.7 * 2523 / pow(2.0,5.0);
    vec3 LMSprime = pow((c1 + c2 * pow(LMS/10000,vec3(n)))/(1 + c3 * pow(LMS/10000,vec3(n))),vec3(p));
    vec3 Izazbz = LMSprime * mat3x3(0.5, 0.5, 0.0, 3.524000, -4.066708, 0.542708, 0.199076, 1.096799, -1.295875);
    float d = -0.56;
    float d0 = 1.6295499532821566 * pow(10.0,-11.0);
    vec3 Jzazbz = Izazbz;
    Jzazbz.x = ((1 + d) * Izazbz.x)/(1 + d * Izazbz.x) - d0;
    return Jzazbz;
}

/* The polar version of Jzazbz */
 
vec3 Jzazbz_to_JzCzhz(vec3 Jzazbz)
{
    float Cz = sqrt(Jzazbz.y*Jzazbz.y + Jzazbz.z*Jzazbz.z);
    float hz = atan(Jzazbz.z,Jzazbz.y);
    vec3 JzCzhz = vec3(Jzazbz.x,Cz,hz);
    return JzCzhz;
}
 
vec3 JzCzhz_Normalize(vec3 JzCzhz)
{
    JzCzhz.x = JzCzhz.x*56.91964;
    JzCzhz.y = JzCzhz.y*40.05235;
    JzCzhz.z = (JzCzhz.z+2.761)/5.522;
    //-2.6274509803921568627450980392157
    //2.760784313725490196078431372549
    //Assume 2.761 both ways so +2.761 then / 5.522
    return JzCzhz;
}
 
vec3 JzCzhz_Denormalize(vec3 JzCzhz)
{
    JzCzhz.x = JzCzhz.x/56.91964;
    JzCzhz.y = JzCzhz.y/40.05235;
    JzCzhz.z = JzCzhz.z * 5.522 - 2.761;
    return JzCzhz;
}
 
vec3 JzCzhz_to_Jzazbz(vec3 JzCzhz)
{
    vec3 Jzazbz = vec3(JzCzhz.x,JzCzhz.y*cos(JzCzhz.z),JzCzhz.y*sin(JzCzhz.z));;
    return Jzazbz;
}
 
vec3 Jzazbz_to_XYZ(vec3 Jzazbz)
{
    float d0 = 1.6295499532821566 * pow(10.0,-11.0);
    float d = -0.56;
    float Iz = (Jzazbz.x + d0) / (1 + d - d * (Jzazbz.x + d0));
    vec3 Izazbz = vec3(Iz,Jzazbz.y,Jzazbz.z);
    vec3 LMSprime = Izazbz * mat3x3(1.0, 0.138605043271539, 0.0580473161561189, 1.0, -0.138605043271539, -0.0580473161561189, 1.0, -0.0960192420263189, -0.811891896056039);
    float c1 = 3424 / pow(2.0,12.0);
    float c2 = 2413 / pow(2.0,7.0);
    float c3 = 2392 / pow(2.0,7.0);
    float n  = 2610 / pow(2.0,14.0);
    float p  = 1.7 * 2523 / pow(2.0,5.0);
    vec3 LMS = 10000 * pow((c1 - pow(LMSprime,vec3(1.0/p)))/(c3 * pow(LMSprime,vec3(1.0/p)) - c2),vec3(1.0/n));
    vec3 XYZprime = LMS * mat3x3(1.92422643578761, -1.00479231259537, 0.037651404030618, 0.350316762094999, 0.726481193931655, -0.065384422948085, -0.0909828109828476, -0.312728290523074, 1.52276656130526);
    float b = 1.15;
    float g = 0.66;
    vec3 XYZ = XYZprime;
    XYZ.x = (XYZprime.x + (b - 1.0) * XYZprime.z) / b;
    XYZ.y = (XYZprime.y + (g - 1.0) * XYZ.x) / g;
    XYZ.z = XYZprime.z;
    return XYZ;
}

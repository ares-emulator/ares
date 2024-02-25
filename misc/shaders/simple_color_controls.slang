#version 450


layout(push_constant) uniform Push
{
	
    float sc_sat,sc_black,sc_temp,sc_sega,sc_brightness,sc_postbr,sc_contrast,
    sc_gamma_out, sc_gamma_in, sc_mono, sc_r,sc_g,sc_b, sc_cs, sc_rg,sc_rb,sc_gb,r_out,g_out,b_out, sc_postdk;
} params;



#pragma parameter sc_cs "Colors: sRGB, PAL, NTSC-U, NTSC-J" 0.0 0.0 3.0 1.0
#pragma parameter sc_temp "Color Temperature in Kelvins (NTSC-J 9300)"  6863.0 1031.0 12047.0 72.0
#pragma parameter sc_gamma_in "Gamma In" 2.4 1.0 4.0 0.05
#pragma parameter sc_rg "Green <-to-> Red Hue" 0.0 -0.25 0.25 0.01
#pragma parameter sc_rb "Blue <-to-> Red Hue"  0.0 -0.25 0.25 0.01
#pragma parameter sc_gb "Blue <-to-> Green Hue" 0.0 -0.25 0.25 0.01
#pragma parameter r_out "CRT Red Vibrancy" 0.29 0.0 1.0 0.01
#pragma parameter g_out "CRT Green Vibrancy" 0.62 0.0 1.0 0.01
#pragma parameter b_out "CRT Blue Vibrancy" 0.09 0.0 1.0 0.01
#pragma parameter sc_brightness "Brightness" 1.0 0.0 2.0 0.01
#pragma parameter sc_contrast "Contrast" 1.0 0.00 2.00 0.01
#pragma parameter sc_sat "Saturation" 1.0 0.0 2.0 0.01
#pragma parameter sc_black  "Black Level" 0.0 -0.20 0.20 0.01 
#pragma parameter sc_sega "Lum Fix: ST/GEN-CPC-AMIGA" 0.0 0.0 3.0 1.0
#pragma parameter sc_postbr "Bright Boost" 1.0 1.0 2.0 0.05
#pragma parameter sc_postdk "Dark Boost" 1.0 1.0 2.0 0.05
#pragma parameter sc_gamma_out "Gamma out" 2.2 1.0 4.0 0.05
#pragma parameter sc_mono "Mono Display On/Off" 0.0 0.0 1.0 1.0
#pragma parameter sc_r "Mono Red/Channel" 1.0 0.0 2.0 0.01
#pragma parameter sc_g "Mono Green/Channel" 1.0 0.0 2.0 0.01
#pragma parameter sc_b "Mono Blue/Channel" 1.0 0.0 2.0 0.01


#define sc_cs params.sc_cs
#define sc_rg params.sc_rg
#define sc_rb params.sc_rb
#define sc_gb params.sc_gb
#define r_out params.r_out
#define g_out params.g_out
#define b_out params.b_out
#define sc_temp params.sc_temp
#define sc_sat params.sc_sat
#define sc_brightness params.sc_brightness
#define sc_contrast params.sc_contrast
#define sc_black params.sc_black
#define sc_sega params.sc_sega
#define sc_postbr params.sc_postbr
#define sc_postdk params.sc_postdk
#define sc_gamma_in params.sc_gamma_in
#define sc_gamma_out params.sc_gamma_out
#define sc_mono params.sc_mono
#define sc_r params.sc_r
#define sc_g params.sc_g
#define sc_b params.sc_b


layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    uint FrameCount;
} global;


#define OutputSize global.OutputSize
#define OriginalSize global.OriginalSize
#define SourceSize global.SourceSize
#define FrameCount global.FrameCount

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

#if defined GL_ES

mat3 PAL = mat3(                    
0.7954  ,   0.1881  ,   0.0053  ,
-0.0310 ,   1.0343  ,   -0.0044 ,
-0.0236 ,   0.1383  ,   0.8927  );

mat3 NTSC = mat3(                   
0.6837  ,   0.2635  ,   0.0336  ,
-0.0499 ,   1.0323  ,   0.0139  ,
-0.0119 ,   0.1071  ,   0.9111  );

mat3 NTSC_J = mat3(                 
0.8642  ,   0.1253  ,   0.0030  ,
0.0545  ,   0.9513  ,   -0.0029 ,
-0.0214 ,   0.1554  ,   0.8750  );

#else
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

// standard 6500k
mat3 NTSC_J = mat3(                 
1.0185  ,   -0.0144 ,   -0.0029 ,
0.0732  ,   0.9369  ,   -0.0059 ,
-0.0318 ,   -0.0080 ,   1.0353  );
#endif


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

mat4 contrastMatrix(float contr)
{   
    float t = (1.0 - contr) / 2.0;
    
    return mat4(contr, 0,               0,               0,
                0,               contr, 0,               0,
                0,               0,               contr, 0,
                t,               t,               t,               1);
}


vec3 toGrayscale(vec3 color)
{

  float average = dot(vec3(0.3,0.59,0.11),color);
  if (sc_cs == 0.0) average = dot(vec3(0.22,0.71,0.07),color);
  return vec3(average);
}

vec3 colorize(vec3 grayscale, vec3 color)
{
    return (grayscale * color);
}


vec3 huePreserveClipDesaturate(float r, float g, float b)
{
   float l = (.299 * r) + (0.587 * g) + (0.114 * b);
   bool ovr = false;
   float ratio = 1.0;

   if ((r > 1.0) || (g > 1.0) || (b > 1.0))
   {
      ovr = true;
      float max = r;
      if (g > max) max = g;
      if (b > max) max = b;
      ratio = 1.0 / max;
   }

   if (ovr)
   {
      r -= 1.0;
      g -= 1.0;
      b -= 1.0;
      r *= ratio;
      g *= ratio;
      b *= ratio;
      r += 1.0;
      g += 1.0;
      b += 1.0;
   }

   r = clamp(r, 0.0, 1.0);
   g = clamp(g, 0.0, 1.0);
   b = clamp(b, 0.0, 1.0);

   return vec3(r, g, b);
}


void main()
{
mat3 hue = mat3(
    1.0, -sc_rg, -sc_rb,
    sc_rg, 1.0, -sc_gb,
    sc_rb, sc_gb, 1.0
);

   vec3 col = texture(Source,vTexCoord).rgb;

  if (sc_sega == 1.0) col = floor(col*7.0+0.5)/7.0;
  if (sc_sega == 2.0) col = floor(col*2.0+0.5)/2.0;
  if (sc_sega == 3.0) col = floor(col*15.0+0.5)/15.0;
   

   col *= sc_brightness;
   
   col = pow((col+0.099)/1.099, vec3(sc_gamma_in));
//color temperature  
   col *= ColorTemp(sc_temp);

   
if (sc_cs != 0.0){
    if (sc_cs == 1.0) col *= PAL;
    if (sc_cs == 2.0) col *= NTSC;
    if (sc_cs == 3.0) col *= NTSC_J;
    col /= vec3(0.24,0.69,0.07);
    col *= vec3(r_out,g_out,b_out); 
    col = clamp(col,0.0,2.0);
}

    col = pow(1.099*col, vec3(1.0/sc_gamma_out))-0.099;
   
    col -= vec3(sc_black);
    col*= vec3(1.0)/vec3(1.0-sc_black);
    
//saturation
vec3 lumw = vec3(0.3,0.59,0.11);
if (sc_cs == 0.0) lumw = vec3(0.2124,0.7011, 0.0866);   
float l = dot(col, lumw);
    
   col = mix(vec3(l), col, sc_sat); 
    if (sc_mono == 1.0)
    {
    vec3 col1 = toGrayscale (col);
    vec3 c = vec3(sc_r, sc_g, sc_b);
    col = colorize (col1, c);
    }
   col *= hue;

col *= mix(sc_postdk,sc_postbr,l);
col = (contrastMatrix(sc_contrast) * vec4(col,1.0)).rgb;  

   col = huePreserveClipDesaturate(col.r, col.g, col.b);

   FragColor = vec4(col,1.0);
}

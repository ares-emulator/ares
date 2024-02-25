
#define kColourSystems  4
#define kPhosphorSets   5

#define kD50            5003.0f
#define kD55            5503.0f
#define kD65            6504.0f
#define kD75            7504.0f
#define kD93            9305.0f

// Input Colour Standards

const mat3 k709_to_XYZ = mat3(
   0.412391f, 0.357584f, 0.180481f,
   0.212639f, 0.715169f, 0.072192f,
   0.019331f, 0.119195f, 0.950532f);

const mat3 kPAL_to_XYZ = mat3(
   0.430554f, 0.341550f, 0.178352f,
   0.222004f, 0.706655f, 0.071341f,
   0.020182f, 0.129553f, 0.939322f);

const mat3 kNTSC_to_XYZ = mat3(
   0.393521f, 0.365258f, 0.191677f,
   0.212376f, 0.701060f, 0.086564f,
   0.018739f, 0.111934f, 0.958385f);

// Phosphor Sets - These override the colour standards above when used

// NTSC-J P22
const mat3 kNTSCJ_P22_to_XYZ = mat3(
   0.458432f, 0.309549f, 0.182474f,
   0.256722f, 0.668848f, 0.074430f,
   0.018337f, 0.127136f, 0.943584f);

//P22 (80’s)
const mat3 kP2280_to_XYZ = mat3(
   0.460844f, 0.307613f, 0.181910f,
   0.244311f, 0.676311f, 0.079378f,
   0.007123f, 0.106901f, 0.975034f);

//P22 (90’s/tinted phosphors)
const mat3 kP2290_to_XYZ = mat3(
   0.404151f, 0.354887f, 0.191418f,
   0.201984f, 0.714530f, 0.083485f,
   0.000607f, 0.062960f, 1.025491f);

//RPTV (late 90’s/early 00’s)
const mat3 kRPTV00_to_XYZ = mat3(
   0.331718f, 0.429476f, 0.189261f,
   0.173634f, 0.738044f, 0.088322f,
   0.012958f, 0.091941f, 0.984159f);

//  NTSC-FCC 1953 Standard Phosphor
const mat3 k1953_to_XYZ = mat3(
   0.588010f, 0.179133f, 0.183224f,
   0.289661f, 0.605640f, 0.104699f,
   0.000000f, 0.068241f, 1.020817f);

// Output Colour Standards

const mat3 kXYZ_to_709 = mat3(
    3.240970f, -1.537383f, -0.498611f,
   -0.969244f,  1.875968f,  0.041555f,
    0.055630f, -0.203977f,  1.056972f);

const mat3 kXYZ_to_DCIP3 = mat3 (
    2.4934969119f, -0.9313836179f, -0.4027107845f,
   -0.8294889696f,  1.7626640603f,  0.0236246858f,
    0.0358458302f, -0.0761723893f,  0.9568845240f);   

const mat3 kStandardsColourGamut[kColourSystems] = { k709_to_XYZ, kPAL_to_XYZ, kNTSC_to_XYZ, kNTSC_to_XYZ };
const mat3 kPhosphorColourGamut[kPhosphorSets] = { kNTSCJ_P22_to_XYZ, kP2280_to_XYZ, kP2290_to_XYZ, kRPTV00_to_XYZ, k1953_to_XYZ };

//const float kTemperatures[kColourSystems] = { kD65, kD65, kD65, kD93 }; 

  // Values from: http://blenderartists.org/forum/showthread.php?270332-OSL-Goodness&p=2268693&viewfull=1#post2268693   
const mat3 kWarmTemperature = mat3(
   vec3(0.0, -2902.1955373783176,   -8257.7997278925690),
	vec3(0.0,  1669.5803561666639,    2575.2827530017594),
	vec3(1.0,     1.3302673723350029,    1.8993753891711275));

const mat3 kCoolTemperature = mat3(
   vec3( 1745.0425298314172,      1216.6168361476490,    -8257.7997278925690),
   vec3(-2666.3474220535695,     -2173.1012343082230,     2575.2827530017594),
	vec3(    0.55995389139931482,     0.70381203140554553,    1.8993753891711275));

const mat4 kCubicBezier = mat4( 1.0f,  0.0f,  0.0f,  0.0f,
                               -3.0f,  3.0f,  0.0f,  0.0f,
                                3.0f, -6.0f,  3.0f,  0.0f,
                               -1.0f,  3.0f, -3.0f,  1.0f );

float Bezier(const float t0, const vec4 control_points)
{
   vec4 t = vec4(1.0, t0, t0*t0, t0*t0*t0);
   return dot(t, control_points * kCubicBezier);
}

vec3 WhiteBalance(float temperature, vec3 colour)
{
   const mat3 m = (temperature < kD65) ? kWarmTemperature : kCoolTemperature;

   const vec3 rgb_temperature = mix(clamp(vec3(m[0] / (vec3(clamp(temperature, 1000.0f, 40000.0f)) + m[1]) + m[2]), vec3(0.0f), vec3(1.0f)), vec3(1.0f), smoothstep(1000.0f, 0.0f, temperature));

   vec3 result = colour * rgb_temperature;

   result *= dot(colour, vec3(0.2126, 0.7152, 0.0722)) / max(dot(result, vec3(0.2126, 0.7152, 0.0722)), 1e-5); // Preserve luminance

   return result;
}

float r601r709ToLinear_1(const float channel)
{
	//return (channel >= 0.081f) ? pow((channel + 0.099f) * (1.0f / 1.099f), (1.0f / 0.45f)) : channel * (1.0f / 4.5f);
	return (channel >= (HCRT_GAMMA_CUTOFF * (1.0f / 1000.0f))) ? pow((channel + 0.099f) * (1.0f / 1.099f), HCRT_GAMMA_IN) : channel * (1.0f / 4.5f);
}

vec3 r601r709ToLinear(const vec3 colour)
{
	return vec3(r601r709ToLinear_1(colour.r), r601r709ToLinear_1(colour.g), r601r709ToLinear_1(colour.b));
}

// XYZ Yxy transforms found in Dogway's Grade.slang shader

vec3 XYZtoYxy(const vec3 XYZ)
{
   const float XYZrgb   = XYZ.r + XYZ.g + XYZ.b;
   const float Yxyg     = (XYZrgb <= 0.0f) ? 0.3805f : XYZ.r / XYZrgb;
   const float Yxyb     = (XYZrgb <= 0.0f) ? 0.3769f : XYZ.g / XYZrgb;
   return vec3(XYZ.g, Yxyg, Yxyb);
}

vec3 YxytoXYZ(const vec3 Yxy)
{
   const float Xs    = Yxy.r * (Yxy.g / Yxy.b);
   const float Xsz   = (Yxy.r <= 0.0f) ? 0.0f : 1.0f;
   const vec3 XYZ    = vec3(Xsz, Xsz, Xsz) * vec3(Xs, Yxy.r, (Xs / Yxy.g) - Xs - Yxy.r);
   return XYZ;
}

const vec4 kTopBrightnessControlPoints    = vec4(0.0f, 1.0f, 1.0f, 1.0f);
const vec4 kMidBrightnessControlPoints    = vec4(0.0f, 1.0f / 3.0f, (1.0f / 3.0f) * 2.0f, 1.0f);
const vec4 kBottomBrightnessControlPoints = vec4(0.0f, 0.0f, 0.0f, 1.0f);

float Brightness(const float luminance)
{
   if(HCRT_BRIGHTNESS >= 0.0f)
   {
      return Bezier(luminance, mix(kMidBrightnessControlPoints, kTopBrightnessControlPoints, HCRT_BRIGHTNESS));
   }
   else
   {
      return Bezier(luminance, mix(kMidBrightnessControlPoints, kBottomBrightnessControlPoints, abs(HCRT_BRIGHTNESS)));
   }
}

const vec4 kTopContrastControlPoints    = vec4(0.0f, 0.0f, 1.0f, 1.0f);
const vec4 kMidContrastControlPoints    = vec4(0.0f, 1.0f / 3.0f, (1.0f / 3.0f) * 2.0f, 1.0f);
const vec4 kBottomContrastControlPoints = vec4(0.0f, 1.0f, 0.0f, 1.0f);

float Contrast(const float luminance)
{
   if(HCRT_CONTRAST >= 0.0f)
   {
      return Bezier(luminance, mix(kMidContrastControlPoints, kTopContrastControlPoints, HCRT_CONTRAST));
   }
   else
   {
      return Bezier(luminance, mix(kMidContrastControlPoints, kBottomContrastControlPoints, abs(HCRT_CONTRAST)));
   }
}

vec3 Saturation(const vec3 colour)
{
   const float luma           = dot(colour, vec3(0.2125, 0.7154, 0.0721));
   const float saturation     = 0.5f + HCRT_SATURATION * 0.5f;

   return clamp(mix(vec3(luma), colour, vec3(saturation) * 2.0f), 0.0f, 1.0f);
}

vec3 BrightnessContrastSaturation(const vec3 xyz)
{
   const vec3 Yxy             = XYZtoYxy(xyz);
   const float Y_gamma        = clamp(pow(Yxy.x, 1.0f / 2.4f), 0.0f, 1.0f);
   
   const float Y_brightness   = Brightness(Y_gamma);

   const float Y_contrast     = Contrast(Y_brightness);

   const vec3 contrast_linear = vec3(pow(Y_contrast, 2.4f), Yxy.y, Yxy.z);
   const vec3 contrast        = clamp(YxytoXYZ(contrast_linear) * kXYZ_to_709, 0.0f, 1.0f);

   const vec3 saturation      = Saturation(contrast);

   return saturation;
}

vec3 ColourGrade(const vec3 colour)
{
   const uint colour_system   = uint(HCRT_CRT_COLOUR_SYSTEM);
   const uint phosphor_set    = uint(HCRT_CRT_PHOSPHOR_SET);

   const float temperature[kColourSystems] = { HCRT_WHITE_TEMPERATURE_D65, HCRT_WHITE_TEMPERATURE_D65, HCRT_WHITE_TEMPERATURE_D65, HCRT_WHITE_TEMPERATURE_D93 };

   const vec3 white_point     = WhiteBalance(temperature[colour_system], colour);

   const vec3 linear          = pow(white_point, vec3(HCRT_GAMMA_IN)); // r601r709ToLinear(white_point); 

   const vec3 xyz             = phosphor_set == 0 ? linear * kStandardsColourGamut[colour_system] : linear * kPhosphorColourGamut[phosphor_set - 1];

   const vec3 graded          = BrightnessContrastSaturation(xyz); 

   return graded;
}

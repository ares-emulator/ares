#ifndef SAT_LUM
#define SAT_LUM

#pragma parameter ia_saturation "Saturation" 1.0 0.0 5.0 0.1
#pragma parameter ia_luminance "Value" 1.0 0.0 2.0 0.1

#include "col_tools.h"

// Note: saturation 0 does not mean grayscale in this case.
//       It's purely a measure of the saturation of the colors.
//       That is, pure r, g and b each look white.

vec3 sat_lum(vec3 in_color){
   vec3 out_color = RGBtoHSV(in_color);
   out_color *= vec3(1.0, ia_saturation, ia_luminance);
   out_color = HSVtoRGB(out_color);
   out_color = clamp(out_color, 0.0, 1.0);
   return out_color;
}

#endif

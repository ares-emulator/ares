#ifndef SAT_LUM
#define SAT_LUM

#pragma parameter ia_saturation "Chrominance" 1.0 0.01 2.0 0.01
#pragma parameter ia_luminance "Luminance" 1.0 0.0 2.0 0.01

#include "col_tools.h"

// Note: This saturation should be similar to broadcast television.
//       0% chrome == pure luma.

vec3 sat_lum(vec3 in_col){
   vec3 out_col = RGBtoYIQ(in_col);
   out_col *= vec3(ia_luminance, ia_saturation, ia_saturation);
   return YIQtoRGB(out_col);
}

#endif

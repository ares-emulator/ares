#ifndef BRIGHT_CON
#define BRIGHT_CON

#pragma parameter ia_contrast "Contrast" 1.0 0.0 10.0 0.05
#pragma parameter ia_bright_boost "Brightness Boost" 0.0 -1.0 1.0 0.05

vec3 cntrst(vec3 in_col){
   vec3 out_col = in_col - 0.5;
   out_col *= ia_contrast;
   out_col += 0.5;
   out_col += ia_bright_boost;
   out_col = clamp(out_col, 0.0, 1.0);
   return out_col;
}

#endif

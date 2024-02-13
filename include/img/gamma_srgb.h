#ifndef GAMMA
#define GAMMA

#pragma parameter gamma_in_lvl "CRT Gamma" 2.4 0.0 3.0 0.05
#pragma parameter gamma_out_lvl "LCD Gamma" 2.2 0.0 3.0 0.05

#include "moncurve.h"

vec3 gamma_in(vec3 in_col){
   vec3 out_col = moncurve_f_f3(in_col, gamma_in_lvl + 0.15, 0.055);
   return out_col;
}

vec3 gamma_out(vec3 in_col){
   vec3 out_col = moncurve_r_f3(in_col, gamma_out_lvl + 0.20, 0.055);
   return out_col;
}

#endif

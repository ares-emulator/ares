#ifndef BLACK_LVL
#define BLACK_LVL

#pragma parameter ia_black_level "Black Level" 0.00 -0.50 0.50 0.01

vec3 black_level(vec3 in_col){
   vec3 out_col = in_col - vec3(ia_black_level);
   out_col *= (vec3(1.0) / vec3(1.0 - ia_black_level));
   return out_col;
}

#endif

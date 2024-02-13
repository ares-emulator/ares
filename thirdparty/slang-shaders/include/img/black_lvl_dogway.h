#ifndef BLACK_LVL
#define BLACK_LVL

#pragma parameter ia_black_level "Black Level" 0.00 -0.50 0.50 0.01

vec3 black_level(vec3 in_col){
   vec3 out_col = in_col;
   out_col += (ia_black_level / 20.0) * (1.0 - out_col);
   return out_col;
}

#endif

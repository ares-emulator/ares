#ifndef BORDER_MASK
#define BORDER_MASK

#pragma parameter ia_TOPMASK "Overscan Mask Top" 0.0 0.0 1.0 0.0025
#pragma parameter ia_BOTMASK "Overscan Mask Bottom" 0.0 0.0 1.0 0.0025
#pragma parameter ia_LMASK "Overscan Mask Left" 0.0 0.0 1.0 0.0025
#pragma parameter ia_RMASK "Overscan Mask Right" 0.0 0.0 1.0 0.0025

vec3 border_mask(vec3 in_col, vec2 coord){
   vec3 out_col = (coord.y > ia_TOPMASK && coord.y < (1.0 - ia_BOTMASK)) ?
      in_col : vec3(0.0);
   out_col = (coord.x > ia_LMASK && coord.x < (1.0 - ia_RMASK)) ?
      out_col : vec3(0.0);
   return out_col;
}

#endif

#ifndef CHANNEL_MIX
#define CHANNEL_MIX

#pragma parameter ia_rr "Red Channel" 1.0 0.0 2.0 0.01
#pragma parameter ia_gg "Green Channel" 1.0 0.0 2.0 0.01
#pragma parameter ia_bb "Blue Channel" 1.0 0.0 2.0 0.01

vec3 channel_mix(vec3 in_col){
   vec3 out_col = in_col * vec3(ia_rr, ia_gg, ia_bb);
   return out_col;
}

#endif

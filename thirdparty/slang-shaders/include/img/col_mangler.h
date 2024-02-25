#ifndef CHANNEL_MIX
#define CHANNEL_MIX

#pragma parameter ia_rr "Red Channel" 1.0 0.0 2.0 0.01
#pragma parameter ia_gg "Green Channel" 1.0 0.0 2.0 0.01
#pragma parameter ia_bb "Blue Channel" 1.0 0.0 2.0 0.01
#pragma parameter ia_rg "Red-Green Tint" 0.0 0.0 1.0 0.005
#pragma parameter ia_rb "Red-Blue Tint" 0.0 0.0 1.0 0.005
#pragma parameter ia_gr "Green-Red Tint" 0.0 0.0 1.0 0.005
#pragma parameter ia_gb "Green-Blue Tint" 0.0 0.0 1.0 0.005
#pragma parameter ia_br "Blue-Red Tint" 0.0 0.0 1.0 0.005
#pragma parameter ia_bg "Blue-Green Tint" 0.0 0.0 1.0 0.005

mat3 mangler = mat3( ia_rr,   ia_rg,   ia_rb,  //red channel
                     ia_gr,   ia_gg,   ia_gb,  //green channel
                     ia_br,   ia_bg,   ia_bb); //blue channel
                     
vec3 channel_mix(vec3 in_col){
   vec3 out_col = in_col * mangler;
   return out_col;
}

#endif

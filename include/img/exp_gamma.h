#ifndef GAMMA
#define GAMMA

#pragma parameter ia_monitor_gamma "Physical Monitor Gamma" 2.2 0.1 5.0 0.05
#pragma parameter ia_target_gamma_r "Simulated Gamma R Channel" 2.2 0.1 5.0 0.05
#pragma parameter ia_target_gamma_g "Simulated Gamma G Channel" 2.2 0.1 5.0 0.05
#pragma parameter ia_target_gamma_b "Simulated Gamma B Channel" 2.2 0.1 5.0 0.05

vec3 gamma_in(vec3 in_col){
   vec3 out_col = pow(in_col, vec3(ia_target_gamma_r, ia_target_gamma_g, ia_target_gamma_b));
   return out_col;
}

vec3 gamma_out(vec3 in_col){
   vec3 out_col= pow(in_col, 1./vec3(ia_monitor_gamma));
   return out_col;
}

#endif

#ifndef VIG
#define VIG

#pragma parameter g_vignette "Vignette Toggle" 1.0 0.0 1.0 1.0
#pragma parameter g_vstr "Vignette Strength" 40.0 0.0 50.0 1.0
#pragma parameter g_vpower "Vignette Power" 0.20 0.0 0.5 0.01

vec3 vignette(vec3 in_col, vec2 in_coord){
    vec2 coord = in_coord * (1.0 - in_coord.xy);
    float vig = coord.x * coord.y * g_vstr;
    vig = min(pow(vig, g_vpower), 1.0);
    vec3 out_col = in_col;
    out_col *= (g_vignette == 1.0) ? vig : 1.0;
    return out_col;
}

#endif

#ifndef SIG_CON
#define SIG_CON

// borrowed from dogway's grade shader

#pragma parameter g_cntrst "Contrast" 0.0 -1.0 1.0 0.05
#pragma parameter g_mid "Contrast Pivot" 0.5 0.0 1.0 0.01
#pragma parameter g_hpfix "Contrast Hotspot Fix" 0.0 0.0 1.0 1.00

#include "col_tools.h"
#include "moncurve.h"

//  Performs better in gamma encoded space
float contrast_sigmoid(float color, float cont, float pivot){

    cont = pow(cont + 1., 3.);

    float knee = 1. / (1. + exp(cont * pivot));
    float shldr = 1. / (1. + exp(cont * (pivot - 1.)));

    color = (1. / (1. + exp(cont * (pivot - color))) - knee) / (shldr - knee);

    return color;
}


//  Performs better in gamma encoded space
float contrast_sigmoid_inv(float color, float cont, float pivot){

    cont = pow(cont - 1., 3.);

    float knee = 1. / (1. + exp (cont * pivot));
    float shldr = 1. / (1. + exp (cont * (pivot - 1.)));

    color = pivot - log(1. / (color * (shldr - knee) + knee) - 1.) / cont;

    return color;
}

//  Saturation agnostic sigmoidal contrast
vec3 cntrst(vec3 in_col){
    vec3 Yxy = XYZtoYxy(sRGB_to_XYZ(in_col));
    float toGamma = clamp(moncurve_r(Yxy.r, 2.40, 0.055), 0.0, 1.0);
    toGamma = (g_hpfix == 0.0) ? toGamma : ((Yxy.r > 0.5) ?
      contrast_sigmoid_inv(toGamma, 2.3, 0.5) : toGamma);
    float sigmoid = (g_cntrst > 0.0) ? contrast_sigmoid(toGamma, g_cntrst, g_mid) : contrast_sigmoid_inv(toGamma, g_cntrst, g_mid);
    vec3 out_col = vec3(moncurve_f(sigmoid, 2.40, 0.055), Yxy.g, Yxy.b);
    vec3 XYZsrgb = clamp(XYZ_to_sRGB(YxytoXYZ(out_col)), 0.0, 1.0);
    out_col = (g_cntrst == 0.0) && (g_hpfix == 0.0) ? in_col :
      ((g_cntrst != 0.0) || (g_hpfix != 0.0) ? XYZsrgb : in_col);
   return out_col;
}

#endif

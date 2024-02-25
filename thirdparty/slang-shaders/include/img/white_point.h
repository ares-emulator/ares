#ifndef WHITE_POINT
#define WHITE_POINT

// White Point Mapping
//          ported by Dogway
//
// From the first comment post (sRGB primaries and linear light compensated)
//      http://www.zombieprototypes.com/?p=210#comment-4695029660
// Based on the Neil Bartlett's blog update
//      http://www.zombieprototypes.com/?p=210
// Inspired itself by Tanner Helland's work
//      http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code

#include "col_tools.h"

#pragma parameter temperature "White Point" 6500.0 1031.0 12047.0 72.0
#pragma parameter luma_preserve "Preserve Luminance" 1.0 0.0 1.0 1.0
#pragma parameter wp_red "Red Shift" 0.0 -1.0 1.0 0.01
#pragma parameter wp_green "Green Shift" 0.0 -1.0 1.0 0.01
#pragma parameter wp_blue "Blue Shift" 0.0 -1.0 1.0 0.01

vec3 wp_adjust(vec3 color){

    float temp = temperature / 100.;
    float k = temperature / 10000.;
    float lk = log(k);

    vec3 wp = vec3(1.);

    // calculate RED
    wp.r = (temp <= 65.) ? 1. : 0.32068362618584273 + (0.19668730877673762 * pow(k - 0.21298613432655075, - 1.5139012907556737)) + (- 0.013883432789258415 * lk);

    // calculate GREEN
    float mg = 1.226916242502167 + (- 1.3109482654223614 * pow(k - 0.44267061967913873, 3.) * exp(- 5.089297600846147 * (k - 0.44267061967913873))) + (0.6453936305542096 * lk);
    float pg = 0.4860175851734596 + (0.1802139719519286 * pow(k - 0.14573069517701578, - 1.397716496795082)) + (- 0.00803698899233844 * lk);
    wp.g = (temp <= 65.5) ? ((temp <= 8.) ? 0. : mg) : pg;

    // calculate BLUE
    wp.b = (temp <= 19.) ? 0. : (temp >= 66.) ? 1. : 1.677499032830161 + (- 0.02313594016938082 * pow(k - 1.1367244820333684, 3.) * exp(- 4.221279555918655 * (k - 1.1367244820333684))) + (1.6550275798913296 * lk);

    // clamp
    wp.rgb = clamp(wp.rgb, vec3(0.), vec3(1.));

    // R/G/B independent manual White Point adjustment
    wp.rgb += vec3(wp_red, wp_green, wp_blue);

    // Linear color input
    return color * wp;
}

vec3 white_point(vec3 in_col){
   vec3 original = sRGB_to_linear(in_col, 2.20);
   vec3 adjusted = wp_adjust(original);
   vec3 base_luma = XYZtoYxy(sRGB_to_XYZ(original));
   vec3 adjusted_luma = XYZtoYxy(sRGB_to_XYZ(adjusted));
   adjusted = (luma_preserve > 0.5) ? adjusted_luma + (vec3(base_luma.r,0.,0.) - vec3(adjusted_luma.r,0.,0.)) : adjusted_luma;
   return linear_to_sRGB(XYZ_to_sRGB(YxytoXYZ(adjusted)), 2.20);
}

#endif

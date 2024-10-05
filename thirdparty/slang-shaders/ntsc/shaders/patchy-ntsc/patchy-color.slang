#version 450

/*
Patchy Color - Consumer Colors Shader
Copyright (C) 2024 Patchy68k/PlainOldPants

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 

*/

layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    vec4 FinalViewportSize;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    
    float pc_unused1;
    float pc_unused2;
    float pc_testpattern;
    float pc_expose_demod_clamp_max;
    float pc_expose_demod_clamp_min;
    float pc_unused3;
    float pc_console;
    float pc_nes_lut;
    float pc_nes_lut_newyuv;
    float pc_nes_fbxcol;
    float pc_nes_fbxcol_sat;
    float pc_nes_version;
//    float pc_nes_alt_voltages;
    float pc_genesis_needfix;
    float pc_unused4;
    float pc_enableComposite;
    float pc_demodulator;
    float pc_r_off;
    float pc_r_amp;
    float pc_g_off;
    float pc_g_amp;
    float pc_unused5;
    float pc_contrast_white_norm;
    float pc_contrast;
    float pc_brightness;
    float pc_color;
    float pc_tint;
    float pc_unused6;
    float pc_gamma_type;
    float pc_g_CRT_l;
    float pc_power_gamma;
} global;

#pragma parameter pc_unused1 "===== Patchy Color Settings =====" 0.0 0.0 0 1

#pragma parameter pc_unused2 "== Test patterns ==" 0 0 0 1
#pragma parameter pc_testpattern "Test pattern selection (0 = off)" 0 0 5 1
//#pragma parameter pc_expose_demod_clamp_max "Reveal clamped color over 1.0 from demodulator conversion" 0 0 1 1
//#pragma parameter pc_expose_demod_clamp_min "Reveal clamped color under 0.0 from demodulator conversion" 0 0 1 1

#pragma parameter pc_unused3 "== Console settings (Does work with test patterns) ==" 0 0 0 1
#pragma parameter pc_console "Console: Other | Gen/MD | NES/FC (raw palette)" 0 0 2 1
#pragma parameter pc_nes_lut "NES: Use real capture (probably 2C02G) | Use formulas" 0 0 1 1
#pragma parameter pc_nes_version "NES PPU formula: 2C02G (NTSC) | 2C02E (NTSC) | PAL" 0 0 2 1
//#pragma parameter pc_nes_alt_voltages "(Reccomend 0 (off)) NES: Use outdated voltages" 0 0 1 1
#pragma parameter pc_genesis_needfix "MD emu: BlastEm (Leave as-is) | GenPlusGX (Apply correction)" 0 0 1 1

#pragma parameter pc_unused4 "=== Composite video demodulation controls ===" 0 0 0 1
#pragma parameter pc_enableComposite "Enable composite demodulator simulation" 1.0 0.0 1.0 1.0
#pragma parameter pc_demodulator "Demodulator (0: BT.470, 1: BT.709, 2-3: JP, 4-8: US, 9: Custom)" 0 0 9 1
#pragma parameter pc_r_off "Custom R-Y offset" 112 90 130 1
#pragma parameter pc_r_amp "Custom R-Y gain" 0.84 0.0 1.00 0.01
#pragma parameter pc_g_off "Custom G-Y offset" 235 200 300 1
#pragma parameter pc_g_amp "Custom G-Y gain" 0.33 0.0 1.00 0.01

#pragma parameter pc_unused5 "=== End-user color tuning knobs ===" 0 0 0 1
#pragma parameter pc_contrast "Contrast (Rec. about 0.8 to 0.9.)" 0.85 0 2 0.025
#pragma parameter pc_brightness "Brightness (Black level)" -0.05 -0.5 0.5 0.01
#pragma parameter pc_color "Color (Saturation)" 1 0 4 0.025
#pragma parameter pc_tint "Tint (Hue Rotation) (Rec. exactly 0.0)" 0 -45 45 0.5

#pragma parameter pc_unused6 "=== Gamma and phosphors ===" 0 0 0 1
#pragma parameter pc_gamma_type "Gamma: BT.1886 + Grade black lift fix | BT.1886 | Power | sRGB" 1 0 3 1
#pragma parameter pc_g_CRT_l        "Black lift fix approximate gamma"                  2.50   2.30  2.60 0.01
#pragma parameter pc_power_gamma "Power gamma" 2.4 2.2 3.0 0.01

#define pi 3.14159265358


vec3 fixGenColor(vec3 src) {
    
    // Code in BlastEm looks like this:
//    static uint8_t levels[] = {0, 27, 49, 71, 87, 103, 119, 130, 146, 157, 174, 190, 206, 228, 255};
//    b = levels[(color >> 8) & 0xE];
//	  g = levels[(color >> 4) & 0xE];
//	  r = levels[color & 0xE];

    // whereas Genesis Plus GX has this:
//    /* 8:8:8 RGB */
//#elif defined(USE_32BPP_RENDERING)
//#define MAKE_PIXEL(r,g,b) ((0xff << 24) | (r) << 20 | (r) << 16 | (g) << 12 | (g)  << 8 | (b) << 4 | (b))
//#endif
//
///* Initialize Mode 5 pixel color look-up tables */
//  for (i = 0; i < 0x200; i++)
//  {
//    /* CRAM 9-bit value (BBBGGGRRR) */
//    r = (i >> 0) & 7;
//    g = (i >> 3) & 7;
//    b = (i >> 6) & 7;
//
//    /* Convert to output pixel format */
//    pixel_lut[0][i] = MAKE_PIXEL(r,g,b);
//    pixel_lut[1][i] = MAKE_PIXEL(r<<1,g<<1,b<<1);
//    pixel_lut[2][i] = MAKE_PIXEL(r+7,g+7,b+7);
//  }

    float levels[] = {0, 27, 49, 71, 87, 103, 119, 130, 146, 157, 174, 190, 206, 228, 255};
    for(int i = 0; i < 3; i++) {
        src[i] = levels[int(min(src[i] * 16 + 0.5, 14.5))] / 255.0;
    }
    return src;

    // Old code, slightly modified from Dogway's Grading Shader (grade.slang) which does not work as of August 2024

//   float lo, hi;
//   for(int i = 0; i < 5; i++) {
//      float cont = 2.578419881;
//      float pivot = 0.520674;
//      float color = (i < 3 ? src[i] : i - 3.0);
//
//      cont = pow(cont - 1., 3.);
//
//      float knee  = 1. / (1. + exp (cont *  pivot));
//      float shldr = 1. / (1. + exp (cont * (pivot - 1.)));
//      
//      float res = pivot - log(1. / (color * (shldr - knee) + knee) - 1.) / cont;
//      if(i < 3)
//         src[i] = res;
//      else if(i == 3)
//         lo = res;
//      else
//         hi = res;
//   }
//   return (src - lo) / (hi - lo);
}

// NES color decoding adapted from GTU-famicom by aliaspider
// Its code is very similar to the code from https://www.nesdev.org/wiki/NTSC_video
// I have updated its code to match the latest version of that code.
bool InColorp (int p, int color)
{
    return ((color + p) % 12 < 6);
}

#define TO_INT2(X) int(floor(((X) * 3.0) + 0.5))
#define TO_INT3(X) int(floor(((X) * 7.0) + 0.5))
#define TO_INT4(X) int(floor(((X) * 15.0) + 0.5))

vec3 createNesColorYuv(vec3 c) {

   int color    = TO_INT4(c.x);
   int level    = TO_INT2(c.y);
   int emphasis = TO_INT3(c.z);

   float black = 0.312;
   float white = 1.100;

   // These voltages were in GTU-famicom by aliaspider.
   // I've found out that they originate from a post by kevtris on nesdev in 2006.
   // https://forums.nesdev.org/viewtopic.php?p=17907#p17907
   float attenuation = 0.746;
   const float gtuFcLevels[8] = float[] (   0.350 , 0.518, 0.962, 1.550,
                                       1.094f, 1.506, 1.962, 1.962);
                                       
   // These new levels originate from lidnariq's post on nesdev in 2015.
   // https://forums.nesdev.org/viewtopic.php?p=159266#p159266
   float levels[16] = float[] (
      0.228f, 0.312f, 0.552f, 0.880f, // Signal low
      0.616f, 0.840f, 1.100f, 1.100f, // Signal high
      0.192f, 0.256f, 0.448f, 0.712f, // Signal low, attenuated
      0.500f, 0.676f, 0.896f, 0.896f  // Signal high, attenuated
   );
   
//   if(global.pc_nes_alt_voltages > 0.5) {
//      for(int i = 0; i < 8; i++) {
//         levels[i] = gtuFcLevels[i];
//         levels[i + 8] = gtuFcLevels[i] * attenuation;
//      }
//      black = .518;
//      white = 1.962;
//   }
   
   if (color > 13)  
      level = 1;

   float signal[12];
   vec3 resultYiq = vec3(0);

   for(int p = 0; p < 12; p++) {
      int amtAtten = 0;

      if (((bool(emphasis & 1) && InColorp(p, 0)) ||
         (bool(emphasis & 2) && InColorp(p, 4)) ||
         (bool(emphasis & 4) && InColorp(p, 8))) && color < 14) 
      {
         amtAtten = 8;
      }

      float low  = levels[0 + level + amtAtten];
      float high = levels[4 + level + amtAtten];

      if(color == 0)
         low = high;
      else if(color > 12)
         high = low;
      
      float sig = InColorp(p, color) ? high : low;

      sig = (sig - black) / (white - black);
      signal[p] = sig;
      resultYiq.x += sig;
   }

   resultYiq.x /= 12;

   // Skew amounts taken from Drag's NES palette generator.
   // I remember seeing these on the nesdev forums somewhere too.
   // Something to be implemented in the future is a different skew amount for the locations that are able to be attenuated, because those are wired differently in the PPU.
   float skew;
   if(global.pc_nes_version < 0.5) {
      // 2C02G
      skew = (3 - level) * -5.0 / 180.0 * pi; // -5 degrees
   } else if(global.pc_nes_version < 1.5) {
      // 2C02E
      skew = (3 - level) * -2.5 / 180.0 * pi; // -2.5 degrees
   } else {
      // In PAL, the alternating phase causes this color error to be cancelled out.
      skew = 0.0;
   }
   
   // Also from lidnariq's post:
   float burstLow = (0.148 - black) / (white - black);
   float burstHigh = (0.524 - black) / (white - black);
   float burstMid = (burstLow + burstHigh) / 2;
   vec2 burstUV = vec2(0);

   for(int i = 0; i < 12; i++) {
      resultYiq.y += (signal[i] - resultYiq.x) * sin((i + 3) * pi / 6 + skew);
      resultYiq.z += (signal[i] - resultYiq.x) * cos((i + 3) * pi / 6 + skew);
      if(i >= 5 && i <= 10) {
          burstUV.x += (burstHigh - burstMid) * sin(i * pi / 6);
          burstUV.y += (burstHigh - burstMid) * cos(i * pi / 6);
      } else {
          burstUV.x += (burstLow - burstMid) * sin(i * pi / 6);
          burstUV.y += (burstLow - burstMid) * cos(i * pi / 6);
      }
   }
   
   resultYiq.yz *= 1.0 / 12.0;
   burstUV *= 1.0 / 12.0;
   
   // 1.1 comes from the NES's white (color 0x20 or 0x30) being at roughly 110 IRE.
   // 0.2 comes from the standard NTSC colorburst amplitude.
   // The result is still quite different from my real hardware capture.
   resultYiq.yz *= (0.2 / 1.1) / sqrt(pow(burstUV.x, 2) + pow(burstUV.y, 2));

   return resultYiq;
}

vec3 lookupNesColor(vec3 nesFormat) {

    //if(global.pc_nes_fbxcol < 0.5) {
    // Color taken from my NES over video capture, with saturation and contrast both at 25% to prevent clamping.
    // The colors were taken via video capture from the 240p test suite.
    // The capture device was an old Dazzle DVC100 that was unused in my house for probably around a decade.
    // I had it plugged into my laptop, and I used OBS to view and screenshot the capture.
    // "Chroma AGC" (presumably, Automatic Gain Control) was enabled, and both contrast and saturation were at 25% to prevent clamping.
    // De-emphasized colors are not included, unfortunately.
        float colorArray[192] = {
            0.478321,   0.478585,   0.478334, 
            0.366470,   0.403287,   0.500952, 
            0.400869,   0.390277,   0.509457, 
            0.429738,   0.383858,   0.496003, 
            0.461984,   0.383553,   0.462478, 
            0.464060,   0.381734,   0.399996, 
            0.456407,   0.392188,   0.335543, 
            0.429468,   0.403545,   0.299086, 
            0.404554,   0.416319,   0.288624, 
            0.360529,   0.423274,   0.301706, 
            0.335662,   0.426387,   0.352844, 
            0.330750,   0.423530,   0.401436, 
            0.341014,   0.418906,   0.454200, 
            0.364416,   0.364810,   0.364445, 
            0.368611,   0.369176,   0.368467, 
            0.368611,   0.369176,   0.368467, 
            0.558871,   0.559355,   0.558862, 
            0.397998,   0.459932,   0.570550, 
            0.445642,   0.440000,   0.594072, 
            0.486899,   0.428728,   0.585057, 
            0.527970,   0.429614,   0.550925, 
            0.537326,   0.425318,   0.467786, 
            0.530366,   0.436656,   0.377826, 
            0.502618,   0.451730,   0.328883, 
            0.470762,   0.468125,   0.308494, 
            0.410478,   0.479321,   0.318534, 
            0.372913,   0.485220,   0.374387, 
            0.361928,   0.482245,   0.437718, 
            0.366940,   0.480164,   0.507869, 
            0.368611,   0.369176,   0.368467, 
            0.368611,   0.369176,   0.368467, 
            0.368611,   0.369176,   0.368467, 
            0.648117,   0.648838,   0.648073, 
            0.479182,   0.551627,   0.647106, 
            0.520994,   0.532234,   0.681346, 
            0.562577,   0.519448,   0.680616, 
            0.602906,   0.518809,   0.653828, 
            0.624149,   0.510424,   0.573186, 
            0.621879,   0.523408,   0.486425, 
            0.598359,   0.535614,   0.428962, 
            0.570986,   0.553840,   0.400425, 
            0.512108,   0.564480,   0.401383, 
            0.471310,   0.572906,   0.444626, 
            0.450232,   0.571801,   0.509053, 
            0.455226,   0.571961,   0.579762, 
            0.454777,   0.455136,   0.454750, 
            0.368611,   0.369176,   0.368467, 
            0.368611,   0.369176,   0.368467, 
            0.648117,   0.648838,   0.648073, 
            0.581149,   0.612569,   0.642884, 
            0.592490,   0.605032,   0.665907, 
            0.610979,   0.600103,   0.666567, 
            0.630066,   0.598694,   0.649834, 
            0.636469,   0.596019,   0.623470, 
            0.641093,   0.600434,   0.590489, 
            0.627745,   0.604366,   0.564505, 
            0.619065,   0.613878,   0.551681, 
            0.592630,   0.617272,   0.548023, 
            0.580770,   0.619989,   0.564797, 
            0.571081,   0.620166,   0.590528, 
            0.568069,   0.622819,   0.618941, 
            0.568471,   0.569152,   0.568223,
            0.368611,   0.369176,   0.368467, 
            0.368611,   0.369176,   0.368467
        };
            
        int color    = TO_INT4(nesFormat.x);
        int level    = TO_INT2(nesFormat.y);
        int emphasis = TO_INT3(nesFormat.z); // De-emphasized colors aren't captured yet.
        
        int colNo = color + level * 16;
        vec3 colRaw = vec3(colorArray[colNo * 3], colorArray[colNo * 3 + 1], colorArray[colNo * 3 + 2]);
        
        float white = colorArray[(2 * 16) * 3]; // NES color 0x20
        float black = colorArray[(1 * 16 + 13) * 3]; // NES color 0x1d
        vec3 res = (colRaw - black) / (white - black);
        
        // Because I haven't captured de-emphasized colors yet, I'm instead using the results from the NES color decoding formulas.
        // Because these RGB values are in BT.709 space, I made sure to set my demodulator to that when doing this.
        // I screenshotted the color 0x30, without de-emphasis, and with each one of the de-emphasis bits set.
        // It would've been possible to get better results if I screenshotted all 8 versions of 0x30.
        // I accidentally used -0.05 brightness and 0.85 contrast when doing this, so I've put a correction for that.
        
        if((emphasis & 1) != 0) {
            // 0xd9d9d9 - 0xd0b6ab = 0x9232e
            res -= vec3(0.03529411764705882, 0.13725490196078433, 0.1803921568627451) / 0.85 / 1.05;
        }
        if((emphasis & 2) != 0) {
            // 0xd9d9d9 - 0xa7c3a8 = 0x321631
            res -= vec3(0.19607843137254902, 0.08627450980392157, 0.19215686274509805) / 0.85 / 1.05;
        }
        if((emphasis & 4) != 0) {
            // 0xd9d9d9 - 0xbbb8e2 = 0x1e21(-09)
            res -= vec3(0.11764705882352941, 0.12941176470588237, -0.03529411764705882) / 0.85 / 1.05;
        }
        
        return res;
    //} else {
    //    // FireBrandX's Composite Direct, only included to test my capture's results.
    //    // This one should never be used by the user because it is clamped. Notice the amount of 0.0s and 1.0s.
    //    // The clamping causes the YUV decoding correction to become incorrect.
    //    float colorArray[192] = {0.396078431372549,
    //        0.396078431372549,
//            0.396078431372549,
//            0,
//            0.07058823529411765,
//            0.49019607843137253,
//            0.09411764705882353,
//            0,
//            0.5568627450980392,
//            0.21176470588235294,
//            0,
//            0.5098039215686274,
//            0.33725490196078434,
//            0,
//            0.36470588235294116,
//            0.35294117647058826,
//            0,
//            0.09411764705882353,
//            0.30980392156862746,
//            0.0196078431372549,
//            0,
//            0.2196078431372549,
//            0.09803921568627451,
//            0,
//            0.11372549019607843,
//            0.19215686274509805,
//            0,
//            0,
//            0.23921568627450981,
//            0,
//            0,
//            0.2549019607843137,
//            0,
//            0,
//            0.23137254901960785,
//            0.09019607843137255,
//            0,
//            0.1803921568627451,
//            0.3333333333333333,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0.6862745098039216,
//            0.6862745098039216,
//            0.6862745098039216,
//            0.09803921568627451,
//            0.3058823529411765,
//            0.7843137254901961,
//            0.2784313725490196,
//            0.1843137254901961,
//            0.8901960784313725,
//            0.4196078431372549,
//            0.12156862745098039,
//            0.8431372549019608,
//            0.5764705882352941,
//            0.10588235294117647,
//            0.6823529411764706,
//            0.6196078431372549,
//            0.10196078431372549,
//            0.3686274509803922,
//            0.6,
//            0.19607843137254902,
//            0,
//            0.4823529411764706,
//            0.29411764705882354,
//            0,
//            0.3568627450980392,
//            0.403921568627451,
//            0,
//            0.14901960784313725,
//            0.47843137254901963,
//            0,
//            0,
//            0.5098039215686274,
//            0,
//            0,
//            0.47843137254901963,
//            0.24313725490196078,
//            0,
//            0.43137254901960786,
//            0.5411764705882353,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            1,
//            1,
//            1,
//            0.39215686274509803,
//            0.6627450980392157,
//            1,
//            0.5568627450980392,
//            0.5372549019607843,
//            1,
//            0.7137254901960784,
//            0.4627450980392157,
//            1,
//            0.8784313725490196,
//            0.43529411764705883,
//            1,
//            0.9372549019607843,
//            0.4235294117647059,
//            0.7686274509803922,
//            0.9411764705882353,
//            0.5019607843137255,
//            0.41568627450980394,
//            0.8470588235294118,
//            0.596078431372549,
//            0.17254901960784313,
//            0.7254901960784313,
//            0.7058823529411765,
//            0.0392156862745098,
//            0.5137254901960784,
//            0.796078431372549,
//            0.047058823529411764,
//            0.3568627450980392,
//            0.8392156862745098,
//            0.24705882352941178,
//            0.2901960784313726,
//            0.8196078431372549,
//            0.49411764705882355,
//            0.30196078431372547,
//            0.7803921568627451,
//            0.796078431372549,
//            0.2980392156862745,
//            0.2980392156862745,
//            0.2980392156862745,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//            1,
//            1,
//            1,
//            0.7803921568627451,
//            0.8980392156862745,
//            1,
//            0.8509803921568627,
//            0.8509803921568627,
//            1,
//            0.9137254901960784,
//            0.8196078431372549,
//            1,
//            0.9764705882352941,
//            0.807843137254902,
//            1,
//            1,
//            0.8,
//            0.9450980392156862,
//            1,
//            0.8313725490196079,
//            0.796078431372549,
//            0.9725490196078431,
//            0.8745098039215686,
//            0.6941176470588235,
//            0.9294117647058824,
//            0.9176470588235294,
//            0.6431372549019608,
//            0.8392156862745098,
//            0.9568627450980393,
//            0.6431372549019608,
//            0.7725490196078432,
//            0.9725490196078431,
//            0.7215686274509804,
//            0.7450980392156863,
//            0.9647058823529412,
//            0.8274509803921568,
//            0.7490196078431373,
//            0.9450980392156862,
//            0.9450980392156862,
//            0.7254901960784313,
//            0.7254901960784313,
//            0.7254901960784313,
//            0,
//            0,
//            0,
//            0,
//            0,
//            0,
//        };
//        
//        int color    = TO_INT4(nesFormat.x);
//        int level    = TO_INT2(nesFormat.y);
//        // int emphasis = TO_INT3(nesFormat.z); // De-emphasized colors aren't captured yet.
//        
//        int colNo = color + level * 16;
//        vec3 colRaw = vec3(colorArray[colNo * 3], colorArray[colNo * 3 + 1], colorArray[colNo * 3 + 2]);
//        
//        float white = colorArray[(2 * 16) * 3]; // NES color 0x20
//        float black = colorArray[(1 * 16 + 13) * 3]; // NES color 0x1d
//
//        return (colRaw - black) / (white - black);
//    }
}

vec3 retreiveNesColorYuv(vec3 nesFormat) {
    mat3 yuvMat;
    //if(global.pc_nes_lut_newyuv < 0.5) {
    //    // https://en.wikipedia.org/wiki/Y%E2%80%B2UV as of 27 Jul 2024
    //    // Standard matrix for BT.470
    //    yuvMat = mat3x3(
    //        0.299, -0.14713, 0.615,
    //        0.587, -0.28886, -0.51499,
    //        0.114, 0.436, -0.10001
    //    );
    //} else {
        // https://en.wikipedia.org/wiki/Y%E2%80%B2UV as of 27 Jul 2024
        // Standard matrix for BT.709
        yuvMat = mat3(0.2126, -0.09991, 0.615,
                      0.7152, -0.33609, -0.55861,
                      0.0722, 0.436, -0.05639);
    //}
    vec3 colYuv;
    if(global.pc_nes_lut < 0.5) {
        // Lookup table (a.k.a. color palette) is enabled.
        colYuv = yuvMat * lookupNesColor(nesFormat);
    } else {
        colYuv = createNesColorYuv(nesFormat);
    }
    if(global.pc_enableComposite < 0.5) {
        return mat3( // BT.470 / Rec. 601 YUV to RGB matrix
    1.0000000, 1.0,  1.0,
    -0.000000029378826483, -0.396552562713623050, 2.031872510910034000,
    1.1383928060531616,  -0.5800843834877014,  0.0000000000000000) * colYuv;
    } else {
        return colYuv;
    }
}

mat3x2 yuvAxisPoints() {
   float r_off, r_max, g_off, g_max, b_off, b_max;
   
   const float Uupscale = 2.03206187221989;
   const float angFix = -123.0 / 180.0 * 3.14159265359;

   if(global.pc_demodulator < 0.5) {
      // Standard BT.470 matrix, used for NTSC and PAL SDTV.
      r_off = (123 - 90) / 180.0 * pi; // 33 deg
      r_max = 1.13983;
      g_off = -1.979; // -113 deg
      g_max = 0.70203;
      b_off = 123 / 180.0 * pi; // 123 deg
      b_max = 2.03211;
  } else if(global.pc_demodulator < 1.5) {
      // Later standard BT.709 matrix, used for NTSC and PAL HDTV.
      r_off = (123 - 90) / 180.0 * 3.14159265359;
      r_max = 1.28033;
      g_off = -2.085;
      g_max = 0.43703;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 2.12798;
   } else if(global.pc_demodulator < 2.5) {
      // Sony CXA2025AS (JP axis) from official documentation, assuming blue is 33 degrees
      r_off = (123 - 95) / 180.0 * 3.14159265359;
      r_max = 0.78;
      g_off = (123 - 240) / 180.0 * 3.14159265359;
      g_max = 0.3;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pc_demodulator < 3.5) {
      // CXA1464AS (JP) from official data sheet
      r_off = (123 - 98) / 180.0 * 3.14159265359;
      r_max = 0.78;
      g_off = (123 - 243) / 180.0 * 3.14159265359;
      g_max = 0.31;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pc_demodulator < 4.5) {
      // Panasonic AN5367FB
      r_off = (123 - 104) / 180.0 * 3.14159265359;
      r_max = 0.96;
      g_off = (123 - 235) / 180.0 * 3.14159265359;
      g_max = 0.36;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pc_demodulator < 5.5) {
      // Toshiba TA8867BN, from official data sheet
      r_off = (123 - 104) / 180.0 * 3.14159265359;
      r_max = 0.91;
      g_off = (123 - 240) / 180.0 * 3.14159265359;
      g_max = 0.31;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pc_demodulator < 6.5) {
      // Toshiba TA8867AN, taken from the official data sheet
      r_off = (123 - 112) / 180.0 * 3.14159265359;
      r_max = 0.84;
      g_off = (123 - 237) / 180.0 * 3.14159265359;
      g_max = 0.33;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pc_demodulator < 7.5) {
      // Sony CXA2025AS (US axis mode) from the official data sheet
      r_off = (123 - 112) / 180.0 * 3.14159265359; // 112 deg or 11 deg
      r_max = 0.83;
      g_off = (123 - 252) / 180.0 * 3.14159265359; // 252 deg or -129 deg
      g_max = 0.3;
      b_off = 123 / 180.0 * 3.14159265359; // 0 deg or 123.5 deg
      b_max = 1.0;
   } else if(global.pc_demodulator < 8.5) {
      // Sony CXA1465AS from the official data sheet
      r_off = (123 - 114) / 180.0 * 3.14159265359;
      r_max = 0.78;
      g_off = (123 - 255) / 180.0 * 3.14159265359;
      g_max = 0.31;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pc_demodulator < 9.5) {
      // Custom from the user, in the same format as in a data sheet.
      r_off = (123 - global.pc_r_off) / 180.0 * pi;
      g_off = (123 - global.pc_g_off) / 180.0 * pi;
      b_off = 123.0 / 180.0 * pi;
      r_max = global.pc_r_amp * Uupscale;
      g_max = global.pc_g_amp * Uupscale;
      b_max = Uupscale;
   }
   
   // PAL matrix from TA8867AN and -BN. Notice that green is more saturated.
   // I'm now assuming that the oversaturated green is a mistake in the data sheet that doesn't appear in real hardware.
   //   r_off = (123 - 90) / 180.0 * 3.14159265359;
   //   r_max = 0.56 * 1.4;
   //   g_off = (123 - 235) / 180.0 * 3.14159265359;
   //   g_max = 0.38 * 1.4;
   //   b_off = 123 / 180.0 * 3.14159265359;
   //   b_max = 1 * 1.4;
   
   r_max *= Uupscale / b_max; // This can be hard-coded into the presets easily.
   g_max *= Uupscale / b_max;
   b_max *= Uupscale / b_max;
   
   r_off += angFix; // This can also be hard-coded into the presets easily.
   g_off += angFix;
   b_off += angFix;
   r_off *= -1;
   g_off *= -1;
   b_off *= -1;
   
   return mat3x2(r_off, r_max,
                 g_off, g_max,
                 b_off, b_max);
}

mat3 yuvAxisMatrix() {

   mat3x2 yuvAxisPts = yuvAxisPoints();
   
//   if(global.pc_console < 1.5) {
//       // This code is made using the mathematical formulas outlined by Chthon at https://forums.libretro.com/t/dogways-grading-shader-slang/27148/561
//       // It might look like we can replace the presets with pre-determined matrices, but if the user sets their own hue offset, we'll need to recalculate the matrix. Not a huge deal though since the code is already fast.
//       // The TV's color and tint dials (saturation and hue rotation) are included in the calculation from yuvAxisPoints() above.
//
//       float xr, yr, xg, yg, xb, yb;
//       xr = yuvAxisPts[0][1] * cos(yuvAxisPts[0][0]);
//       yr = yuvAxisPts[0][1] * sin(yuvAxisPts[0][0]);
//       xg = yuvAxisPts[1][1] * cos(yuvAxisPts[1][0]);
//       yg = yuvAxisPts[1][1] * sin(yuvAxisPts[1][0]);
//       xb = yuvAxisPts[2][1] * cos(yuvAxisPts[2][0]);
//       yb = yuvAxisPts[2][1] * sin(yuvAxisPts[2][0]);
//       
//       float wr = 0.298911657927057;
//       float wg = 0.586610718748869;
//       float wb = 0.114477623324074;
//       
//       float ud = 0.492111;
//       float vd = 0.877283;
//       float uu = 2.03206187221989;
//       float vu = 1.13988302520395;
//       
//       mat2 matB = mat2(
//          (1-wr)/vu, -wg/vu,
//          -wr/uu, -wg/uu
//       );
//       
//       vec2 cr = matB * vec2(yr, xr);
//       vec2 cg = matB * vec2(yg, xg);
//       vec2 cb = matB * vec2(yb, xb);
//       
//       return mat3(
//          cr.r + wr, cg.r + wr, cb.r + wr,
//          cr.g + wg, cg.g + wg, cb.g + wg,
//          1 - (cr.r + wr + cr.g + wg), 1 - (cg.r + wr + cg.g + wg), 1 - (cb.r + wr + cb.g + wg)
//       );
//   } else {

    // This simpler code generates a YUV to RGB matrix.
    float xr, yr, xg, yg, xb, yb;
    xr = yuvAxisPts[0][1] * cos(yuvAxisPts[0][0]);
    yr = yuvAxisPts[0][1] * sin(yuvAxisPts[0][0]);
    xg = yuvAxisPts[1][1] * cos(yuvAxisPts[1][0]);
    yg = yuvAxisPts[1][1] * sin(yuvAxisPts[1][0]);
    xb = yuvAxisPts[2][1] * cos(yuvAxisPts[2][0]);
    yb = yuvAxisPts[2][1] * sin(yuvAxisPts[2][0]);

    mat3 toRgb = mat3(1,  1,  1,
                      xr, xg, xb,
                      yr, yg, yb);
    
    mat3 fromRgb = inverse(toRgb);
    mat3 fromRgbStd = mat3(0.299, -0.14713, 0.615, // Standard YUV matrix from BT.470
                           0.587, -0.28886, -0.51499,
                           0.114, 0.436, -0.10001);
                           
    // Basic linear regression to match the hue to the color bars.
    
    float residSum = 0;
    for(int i = 1; i <= 6; i++) {
        vec3 colorBar = vec3(i & 1, (i >> 1) & 1, i >> 2);
        vec3 yuvJungle = fromRgb * colorBar;
        vec3 yuvStd = fromRgbStd * colorBar;
        float resid = atan(yuvJungle.b, yuvJungle.g) - atan(yuvStd.b, yuvStd.g);
        while(resid > pi)
            resid -= 2 * pi;
        while(resid < -pi)
            resid += 2 * pi;
        residSum += resid;
    }
    
    float hueOff = -residSum / 6;
    
    for(int i = 0; i < 3; i++)
        yuvAxisPts[i][0] += hueOff;
    
    
    xr = yuvAxisPts[0][1] * cos(yuvAxisPts[0][0]);
    yr = yuvAxisPts[0][1] * sin(yuvAxisPts[0][0]);
    xg = yuvAxisPts[1][1] * cos(yuvAxisPts[1][0]);
    yg = yuvAxisPts[1][1] * sin(yuvAxisPts[1][0]);
    xb = yuvAxisPts[2][1] * cos(yuvAxisPts[2][0]);
    yb = yuvAxisPts[2][1] * sin(yuvAxisPts[2][0]);

    toRgb = mat3(1,  1,  1,
                 xr, xg, xb,
                 yr, yg, yb);
    
    fromRgb = inverse(toRgb);

    // Old least squares linear regression to match the hue to the color bars.
    
//    float Aval = 0;
//    float Bval = 0;
//    for(int i = 1; i <= 6; i++) {
//        vec3 colorBar = vec3(i & 1, (i >> 1) & 1, i >> 2);
//        vec3 yuvJungle = fromRgb * colorBar;
//        vec3 yuvStd = fromRgbStd * colorBar;
//        Aval += pow(yuvJungle.g, 2) + pow(yuvJungle.b, 2);
//        Bval += 2 * sqrt(pow(yuvJungle.g, 2) + pow(yuvJungle.b, 2)) * -sqrt(pow(yuvStd.g, 2) + pow(yuvStd.b, 2));
//    }
//    
//    float satFix = -Bval / (2 * Aval);

    // New geometric mean to match the saturation to the color bars.
    // Take the proportion of the change on each color bar.
    // Then, take the geometric mean of those six proportions.
    // This has a better result than the linear regression above because saturation is not linear.
    
    float prop = 1.0;
    for(int i = 1; i <= 6; i++) {
        vec3 colorBar = vec3(i & 1, (i >> 1) & 1, i >> 2);
        vec3 yuvJungle = fromRgb * colorBar;
        vec3 yuvStd = fromRgbStd * colorBar;
        float satJungle = sqrt(pow(yuvJungle.g, 2) + pow(yuvJungle.b, 2));
        float satStd = sqrt(pow(yuvStd.g, 2) + pow(yuvStd.b, 2));
        prop *= satJungle / satStd; // What saturation do we set to make this bar match in saturation?
    }
    
    prop = pow(prop, 1.0/6.0); // Geometric mean of those proportional errors.
    
    return toRgb * mat3(
        1, 0, 0,
        0, prop, 0,
        0, 0, prop
    );
    
//   }
}

float nesMaxRgb() {
   mat3x2 yuvAxisPts = yuvAxisPoints();
   
   float maxAmp = max(yuvAxisPts[0][1], max(yuvAxisPts[1][1], yuvAxisPts[2][1]));
   
   // This can be sped up by using precomputed values.
   // For speed, I am skipping the emphasis bits.
   float maxRgb = 0.0;
   for(int i = 0; i < 4; i++) {
      vec3 yiqVal = createNesColorYuv(vec3(1.0 / 15.0, i / 3.0, 0.0));
      float yVal = yiqVal.r;
      float ampVal = sqrt(pow(yiqVal.g, 2) + pow(yiqVal.b, 2));
      maxRgb = max(maxRgb, yVal + maxAmp * ampVal);
   }
   
   return max(maxRgb, 1.0); // 1.0 is the maximum pure white on the NES.
}

float EOTF_1886a_default(float color, float brightness, float contrast) {
    // From Rec. ITU-R BT.1886
    float Vc = 0.35, // Vc, a1, and a2 are directly from the paper
          a1 = 2.6,
          a2 = 3.0,
          Lw = contrast;
    
    float V = color,
          b = brightness, // The paper recommends about either 0 or 0.1.
          k = Lw / pow(1.0 + b, a1);
    
    if(V < Vc) {
        return k * pow(Vc + b, a1 - a2) * pow(V + b, a2);
    } else {
        return k * pow(V + b, a1);
    }
}
 
vec3 EOTF_1886a_default_f3(vec3 color, float brightness, float contrast) {
    color.r = EOTF_1886a_default(color.r, brightness, contrast);
    color.g = EOTF_1886a_default(color.g, brightness, contrast);
    color.b = EOTF_1886a_default(color.b, brightness, contrast);
    return color;
}

// CRT EOTF Function
// Taken from Grade
//----------------------------------------------------------------------

#define CRT_l           -(100000.*log((72981.-500000./(3.*max(2.3,global.pc_g_CRT_l)))/9058.))/945461.

float EOTF_1886a(float color, float bl, float brightness, float contrast) {

    // Defaults:
    //  Black Level = 0.1
    //  Brightness  = 0
    //  Contrast    = 100

    const float wl = 100.0;
          float b  = pow(bl, 1./2.4);
          float a  = pow(wl, 1./2.4)-b;
//                b  = (brightness-50.) / 250. + b/a;                   // -0.20 to +0.20
                b  = (brightness * 100.) / 250. + b/a;                   // -0.20 to +0.20
                //a  = contrast!=50. ? pow(2.,(contrast-50.)/50.) : 1.; //  0.50 to +2.00
                a = contrast;

    const float Vc = 0.35;                           // Offset
          float Lw = wl/100. * a;                    // White level
          float Lb = min( b  * a,Vc);                // Black level
    const float a1 = 2.6;                            // Shoulder gamma
    const float a2 = 3.0;                            // Knee gamma
          float k  = Lw /pow(1. + Lb,    a1);
          float sl = k * pow(Vc + Lb, a1-a2);        // Slope for knee gamma

    color = color >= Vc ? k * pow(color + Lb, a1 ) : sl * pow(color + Lb, a2 );

    // Black lift compensation
    float bc = 0.00446395*pow(bl,1.23486);
    color    = min(max(color-bc,0.0)*(1.0/(1.0-bc)), 1.0);  // Undo Lift
    color    = pow(color,1.0-0.00843283*pow(bl,1.22744));   // Restore Gamma from 'Undo Lift'

    return color;
 }

vec3 EOTF_1886a_f3( vec3 color, float BlackLevel, float brightness, float contrast) {

    color.r = EOTF_1886a( color.r, BlackLevel, brightness, contrast);
    color.g = EOTF_1886a( color.g, BlackLevel, brightness, contrast);
    color.b = EOTF_1886a( color.b, BlackLevel, brightness, contrast);
    return color.rgb;
 }

vec3 smpteBars(vec2 pos) {
   float wholeRamp = pos.x * 8;
   float flooredWholeRamp = floor(pos.x * 8);
   if(pos.y < 0.75) {
      // Standard color bars
      int index = 7 - int(flooredWholeRamp + 0.5);
      return vec3((index & 2) >> 1, index >> 2, index & 1);
   } else if(pos.x > 0.875) {
      // Standard mini-pluge for syncing the black level
      float partialRamp = wholeRamp - flooredWholeRamp;
      float steppedPartialRamp = floor(partialRamp * 3);
      return vec3((steppedPartialRamp - 1.0) * 0.075);
   } else {
      // Deeper pluge for setting a nonstandard brightness
      float normPos = pos.x / 0.875;
      float normY = (pos.y - 0.75) / 0.25;

      float steppedPos = floor(normPos * 21);
      float finalRamp = (steppedPos - 10) / 10;
      
      if(normY < 0.25) {
         return vec3(finalRamp, 0, 0);
      } else if(normY < 0.5) {
         return vec3(0, finalRamp, 0);
      } else if(normY < 0.75) {
         return vec3(0, 0, finalRamp);
      } else {
         return vec3(finalRamp);
      }
   }
}

vec3 colorRamps(vec2 pos) {
    int x = int(pos.x * 17);
    int y = int(pos.y * 15);
    
    if(x == 0 || x == 16 || (y & 1) == 0) {
        return vec3(0);
    } else {
        // Red, green, blue, and white are grouped together on purpose.
        vec3 primaries[] = {
            vec3(1, 1, 1),
            vec3(1, 0, 0),
            vec3(0, 1, 0),
            vec3(0, 0, 1),
            vec3(0, 1, 1),
            vec3(1, 0, 1),
            vec3(1, 1, 0)
        };
        return mix(vec3(0), primaries[y >> 1], x / 15.0);
    }
}

vec3 hsvSpectrum(vec2 pos, bool widenKeyAreas) {
   vec3 black = vec3(0);
   vec3 white = vec3(1);
   vec3 primaries[] = {
      vec3(1, 0, 1),
      vec3(1, 0, 0),
      vec3(1, 1, 0),
      vec3(0, 1, 0),
      vec3(0, 1, 1),
      vec3(0, 0, 1),
      vec3(1, 0, 1), // Colors are repeated intentionally
      vec3(1, 0, 0),
      vec3(1, 1, 0),
   };

   float scaledY = pos.y * 7.0 + 0.5;
   int lowIndex = int(floor(scaledY) + 0.1);
   int highIndex = int(ceil(scaledY) + 0.1);
   float interpolation = scaledY - lowIndex;
   if(widenKeyAreas)
      interpolation = min(interpolation * 1.5, 1.0);
   vec3 mixedCenter = mix(primaries[lowIndex], primaries[highIndex], interpolation);

   float scaledX = pos.x * 2;
   if(scaledX < 1) {
      if(widenKeyAreas)
         scaledX = min(scaledX * 1.5, 1.0);
      return mix(black, mixedCenter, scaledX);
   } else {
      return mix(mixedCenter, white, scaledX - 1);
   }
}

vec3 nesFullPalette(vec2 pos) {
   float hue = floor(pos.x * 16.0) / 15.0;
   float emphasis = floor(pos.y * 8.0) / 7.0;
   float level = floor(4.0 * (pos.y * 8.0 - floor(pos.y * 8.0))) / 3.0;
   return vec3(hue, level, emphasis);
}

vec3 testPattern(vec2 pos) {
    if(global.pc_testpattern < 2.5) {
        return colorRamps(pos);
    } else if(global.pc_testpattern < 3.5) {
        return hsvSpectrum(pos, false);
    } else if(global.pc_testpattern < 4.5) {
        return hsvSpectrum(pos, true);
    } else if(global.pc_testpattern < 5.5) {
        return nesFullPalette(pos);
    }
    else return smpteBars(pos);
}

// These two sRGB functions are taken from shaders_slang/include/colorspace-tools.h
// Colorspace Tools
// ported from Asmodean's PsxFX Shader Suite v2.00
// License: GPL v2+
vec3 linear_to_sRGB(vec3 color, float gamma)
{
    color = clamp(color, 0.0, 1.0);
    color.r = (color.r <= 0.00313066844250063) ?
    color.r * 12.92 : 1.055 * pow(color.r, 1.0 / gamma) - 0.055;
    color.g = (color.g <= 0.00313066844250063) ?
    color.g * 12.92 : 1.055 * pow(color.g, 1.0 / gamma) - 0.055;
    color.b = (color.b <= 0.00313066844250063) ?
    color.b * 12.92 : 1.055 * pow(color.b, 1.0 / gamma) - 0.055;

    return color.rgb;
}

vec3 sRGB_to_linear(vec3 color, float gamma)
{
    color = clamp(color, 0.0, 1.0);
    color.r = (color.r <= 0.04045) ?
    color.r / 12.92 : pow((color.r + 0.055) / (1.055), gamma);
    color.g = (color.g <= 0.04045) ?
    color.g / 12.92 : pow((color.g + 0.055) / (1.055), gamma);
    color.b = (color.b <= 0.04045) ?
    color.b / 12.92 : pow((color.b + 0.055) / (1.055), gamma);

    return color.rgb;
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 v_texcoord0;

void main()
{
   gl_Position = global.MVP * Position;
   v_texcoord0 = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 v_texcoord0;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec4 originalColor = texture(Source, v_texcoord0.xy);

    if(global.pc_testpattern > 0.5) {
        originalColor = vec4(testPattern(v_texcoord0.xy), 1);
    }
    
    vec3 changedColor = originalColor.rgb;
    
    if(global.pc_console > 0.5) {
        if(global.pc_console < 1.5) { // Sega Genesis / MegaDrive
            if(global.pc_genesis_needfix > 0.5 && global.pc_testpattern < 0.5) // Genesis gamma correction--BlastEm already performs this fix for you
                changedColor = fixGenColor(changedColor);
        }
            else if(global.pc_console < 2.5) { // NES PPU
                changedColor = retreiveNesColorYuv(changedColor);
        }
    }

    if(global.pc_enableComposite > 0.5) {
        
        mat3 toYuv = mat3(0.299, -0.14713, 0.615, // Standard YUV matrix from BT.470
                          0.587, -0.28886, -0.51499, // Used in both NTSC and PAL.
                          0.114, 0.436, -0.10001);
        mat3 toRgb = yuvAxisMatrix();
        
        if(global.pc_console < 1.5)
            changedColor = toYuv * changedColor;
            
        float tint = -global.pc_tint * pi / 180.0;
        changedColor.gb *= mat2(
            cos(tint), -sin(tint),
            sin(tint), cos(tint)
        );
        changedColor.gb *= global.pc_color;
        
        changedColor = toRgb * changedColor;
    }
    
    if(global.pc_gamma_type < 0.5) {
        changedColor = EOTF_1886a_f3(changedColor, CRT_l, global.pc_brightness, global.pc_contrast);
    } else if(global.pc_gamma_type < 1.5) {
        changedColor = EOTF_1886a_default_f3(changedColor, global.pc_brightness, global.pc_contrast);
    } else if(global.pc_gamma_type < 2.5) {
        changedColor = pow((changedColor + global.pc_brightness) / (1 + global.pc_brightness), vec3(global.pc_power_gamma)) * global.pc_contrast;
    } else {
        float contr = global.pc_contrast <= 1.001 ? linear_to_sRGB(vec3(global.pc_contrast), 2.4).r : 1.055 * pow(global.pc_contrast, 1.0 / 2.4) - 0.055; // 2.4 instead of 2.2 is not a mistake.
        changedColor = (changedColor + global.pc_brightness) / (1 + global.pc_brightness) * contr;
        changedColor = sRGB_to_linear(changedColor, 2.4); // Beware that this function clamps between 0 and 1.
    }
    
    FragColor = vec4(clamp(changedColor, 0, 1), originalColor.a);
}


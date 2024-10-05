#version 450

/*
Patchy NTSC
Copyright (C) 2024 Patchy68k/PlainOldPants

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 

*/

// This pass generates the raw, unfiltered luma and chroma signals separately. An optional fix for Genesis colors is included.

layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    vec4 FinalViewportSize;
} params;

// Includes constants, global, and all parameters
#include "patchy-ntsc-inc-params.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

// Includes lowpass and bandpass functions and some shared code
#include "patchy-ntsc-inc-filters.inc"

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

    // Old code from Dogway's Grading Shader (grade.slang) which does not work as of August 2024

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

bool InColorp (int p, int color)
{
    return ((color + p) % 12 < 6);
}

#define TO_INT2(X) int(floor(((X) * 3.0) + 0.5))
#define TO_INT3(X) int(floor(((X) * 7.0) + 0.5))
#define TO_INT4(X) int(floor(((X) * 15.0) + 0.5))

vec3 smpteBars(vec2 pos) {
   float wholeRamp = pos.x * 8;
   float flooredWholeRamp = floor(pos.x * 8);
   if(pos.y < 0.75) {
      // Standard color bars. Top is 75%; bottom is 100%.
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
    if(global.pn_test_pattern < 1.5) {
        return smpteBars(pos);
    } else if(global.pn_test_pattern < 2.5) {
        return colorRamps(pos);
    } else if(global.pn_test_pattern < 3.5) {
        return hsvSpectrum(pos, false);
    } else if(global.pn_test_pattern < 4.5) {
        return hsvSpectrum(pos, true);
    } else if(global.pn_test_pattern < 5.5) {
        return nesFullPalette(pos);
    }
}

void main()
{
    vec4 colorIn;
    
    if(global.pn_test_pattern > 0.5) {
        colorIn.rgb = testPattern(vTexCoord);
    } else if(global.pn_nes_enable < 0.5 && global.pn_rgb_blur_enable > 0.5) {
        int xscale = int(getSizes().x / global.pn_width_uncropped + 0.5);
        vec3 sum = vec3(0);
        int size = int(xscale * SIGNAL_RES * global.pn_rgb_blur_amount + 0.5);
        int start = (-size) / 2; // rounds up towards 0
        int end = start + size;
        for(int i = start; i < end; i++) {
            sum += texture(Source, vTexCoord - vec2(i * params.OutputSize.z, 0)).rgb;
        }
        colorIn.rgb = sum / size;
    } else {
        colorIn = texture(Source, vTexCoord);
    }
    
    // Compensate for cropped overscan, assuming equal crop on both sides. We don't know whether the user is cropping overscan or not.
    // Current position in pixels. X is a decimal number. Y is floored to an integer.
    vec2 pixelCoord = realPixelCoord();
    
    if(global.pn_nes_enable > 0.5) {
        
        // This code is taken from both the nesdev wiki and from GTU-famicom, the latter created by aliaspider.
        
        int p = getPhaseIntNES();
        
        int color    = TO_INT4(colorIn.x);
        int level    = TO_INT2(colorIn.y);
        int emphasis = TO_INT3(colorIn.z);
   
        if(color > 13)  
            level = 1;

        float black = 0.312;
        float white = 1.100;
        
        // These new levels originate from lidnariq's post on nesdev in 2015.
        // https://forums.nesdev.org/viewtopic.php?p=159266#p159266
        float levels[16] = float[] (
            0.228f, 0.312f, 0.552f, 0.880f, // Signal low
            0.616f, 0.840f, 1.100f, 1.100f, // Signal high
            0.192f, 0.256f, 0.448f, 0.712f, // Signal low, attenuated
            0.500f, 0.676f, 0.896f, 0.896f  // Signal high, attenuated
        );
        
        int amtAtten = 0;

        if(((bool(emphasis & 1) && InColorp(p, 0)) ||
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
        
        FragColor = vec4(sig, 0, 0, 1);
        
    } else {
        
        if(global.pn_genesis_palette > 0.5 && global.pn_test_pattern < 0.5) {
            colorIn.rgb = fixGenColor(colorIn.rgb);
        }
        
        float phase = getPhase();
        
        mat3 yuvMat;
        if(global.pn_modulator_std < 0.5) {
            // Rec. 601 YUV matrix
            // This is used in the Genesis/MegaDrive and SNES/SFC
            yuvMat = mat3x3(
                        0.299, -0.14713, 0.615,
                        0.587, -0.28886, -0.51499,
                        0.114, 0.436, -0.10001
                     );
        } else if(global.pn_modulator_std < 1.5) {
            // Rec. 709 YUV matrix
            // Don't know any specific consoles using this, but I assume this *eventually* became common.
            yuvMat = mat3(0.2126, -0.09991, 0.615,
                          0.7152, -0.33609, -0.55861,
                          0.0722, 0.436, -0.05639);
        } else {
            // This matrix was removed because it's too similar to Rec. 601, and it is less precise due to
            // the data sheets only having two decimal places of precision.
            
//            float redShift, redAmp, redY,
//            greenShift, greenAmp, greenY,
//            blueShift, blueAmp, blueY,
//            whiteY;
//   
//          // NTSC color encoding
//          // Genesis/MegaDrive's Sony CXA1145
//          // Taken from Sony's official documentation
//          // Also, the CXA1645, MB3514, and KA2195D all have these same values too.
//          // Same values are also found in the hard-to-find datasheets for SNES video encoders.
//          // It's safe to assume almost any 90s console uses these values for composite.
//          // The factor 0.2 comes from the standard colurburst amplitude being 0.2.
//          // This can be verified against the data sheets, as their colorburst amplitude is 0.29 / 0.71 / 2.
//          redShift = (104.0) / 180.0 * pi;
//          redAmp = 3.16 * 0.2;
//          redY = 0.21;
//          greenShift = (-119.0) / 180.0 * pi; // -119 deg = 241 deg
//          greenAmp = 2.95 * 0.2;
//          greenY = 0.42;
//          blueShift = (-13.0) / 180.0 * pi; // -13 deg = 347 deg
//          blueAmp = 2.24 * 0.2;
//          blueY = 0.08;
//          whiteY = 0.71;
//
//          yuvMat = mat3(
//             redY/whiteY,   redAmp*cos(redShift),      redAmp*sin(redShift),
//             greenY/whiteY, greenAmp*cos(greenShift),  greenAmp*sin(greenShift),
//             blueY/whiteY,  blueAmp*cos(blueShift),    blueAmp*sin(blueShift)
//          );
        }
        
        vec3 yuvCol = yuvMat * colorIn.rgb;
        
        if(global.pn_connection_type < 1.5) {
            // Composite and S-Video
        
            float luma = yuvCol.r / global.pn_color_amplitude;
            float chroma = (yuvCol.g * sin(phase) + yuvCol.b * cos(phase));
            
            FragColor = vec4(luma, chroma, 0, 1);
        } else if(global.pn_connection_type < 2.5) {
            // No Artifacts
            FragColor = vec4(yuvCol, 1);
        }
    }
}


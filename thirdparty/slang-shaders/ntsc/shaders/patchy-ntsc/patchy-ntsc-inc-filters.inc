
/*
Patchy NTSC
Copyright (C) 2024 Patchy68k/PlainOldPants

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 

*/

#ifndef __patchy_ntsc_inc_filters_inc__
#define __patchy_ntsc_inc_filters_inc__

///////////////////////
// General functions //
///////////////////////

// NES simulation requires SIGNAL_RES to be exactly 8. Not 16. Not 4. Only 8.
// Some code involving NES has 8 hardcoded, and some code uses SIGNAL_RES instead.
#define SIGNAL_RES global.pn_signal_res
#define pi 3.14159265358

// The initial release of Patchy NTSC kept multiplying things by SIGNAL_RES without accounting for changes in resolution, colorburst rate, and active area duration.
// These macros combine all those corrections into one macro, based on the initial release's Sega Genesis settings.
// Always be careful about whether you use SIGNAL_RES or SIGNAL_RES_CORRECTION.
#define SETTINGS_CORRECTION (getSizes().x / 320.0 * 47.7 / global.pn_scanline_dur * 3.5795454 / global.pn_color_freq)
#define SIGNAL_RES_CORRECTION (SIGNAL_RES * SETTINGS_CORRECTION)

// Get the cropped and uncropped sizes of the screen, taking into account the possibility that the resolution was multiplied by an exact integer.
vec4 getSizes() {
    vec2 sizeUncropped = vec2(global.pn_width_uncropped, global.pn_height_uncropped);
    vec2 sizeCropped = params.OutputSize.xy / vec2(SIGNAL_RES, 1);
    
    // Correct for integer-scaled resolutions.
    if(global.pn_width_integer_mul > 0.5)
        sizeUncropped.x *= floor(0.501 + sizeCropped.x / sizeUncropped.x);
    if(global.pn_height_integer_mul > 0.5)
        sizeUncropped.y *= floor(0.501 + sizeCropped.y / sizeUncropped.y);
    
    if(global.pn_dual_width_mode > 0.5) {
        float smallWidthUncropped = global.pn_width_secondary_uncropped;
        
        // Correct for integer-scaled resolutions.
        if(global.pn_width_integer_mul > 0.5)
            smallWidthUncropped *= floor(0.501 + sizeCropped.x / smallWidthUncropped);
        
        // Use whichever resolution we're closer to, using the absolute value of the difference.
        float smallDiff = abs(sizeCropped.x - smallWidthUncropped);
        float largeDiff = abs(sizeCropped.x - sizeUncropped.x);
        if(smallDiff < largeDiff)
            sizeUncropped.x = smallWidthUncropped;
    }
    
    return vec4(sizeUncropped, sizeCropped);
}

// Find which scanline we're on, and what percent of the active area has been completed.
// This takes cropped overscan and changed resolution into account, under three assumptions:
//      1. An equal amount of overscan was cropped on the left and right sides.
//      2. The top and bottom sides either had an equal amount cropped or had exactly one more pixel cropped off the bottom than on the top.
//      3. The resulution, horizontal or vertical, might have been multiplied by an exact integer. (For example, in bsnes-mercury.)
vec2 realPixelCoord() {
    vec4 sizes = getSizes();
    vec2 sizeUncropped = sizes.xy;
    vec2 sizeCropped = sizes.zw;
    vec2 pixelCoord = vTexCoord * sizeCropped + (sizeUncropped - sizeCropped) / 2.0;
    pixelCoord.y = floor(pixelCoord.y + 0.01);
    return pixelCoord;
}

int getPhaseIntNES(int yOff) {
    vec2 pixelCoord = realPixelCoord();
    return int(0.1 + 8 * pixelCoord.x + 4 * (pixelCoord.y + yOff) + 4 * (int(params.FrameCount + 0.1) % int(2.1 + global.pn_nes_phase_mod)));;
}

int getPhaseIntNES() {
    return getPhaseIntNES(0);
}

float getPhase(int yOff) {
    if(global.pn_nes_enable > 0.5) {
        int p = getPhaseIntNES(yOff);
        return 2 * pi * (p + 3) / 12.0;
    } else {
        vec2 pixelCoord = realPixelCoord();
        float realOffX = pixelCoord.x / getSizes().x;
        return 2 * pi * (realOffX * global.pn_scanline_dur * global.pn_color_freq + global.pn_color_init_offset + global.pn_color_line_offset * (pixelCoord.y + yOff) + (global.pn_color_screen_offset_modulo < 1.5 ? params.FrameCount : params.FrameCount % int(global.pn_color_screen_offset_modulo + 0.5)) * global.pn_color_screen_offset);
    }
}

float getPhase() {
    return getPhase(0);
}

float getFreqFactor() {
    if(global.pn_nes_enable > 0.5) {
        // https://www.nesdev.org/wiki/NTSC_video as of August 15, 2024
        // Each NES pixel is 186 nanoseconds.
        return 0.186 / 8.0 * 2 * pi;
    } else {
        return global.pn_scanline_dur / getSizes().x / SIGNAL_RES * 2 * pi;
    }
}

float getBurstRate() {
    if(global.pn_nes_enable > 0.5) {
        return 2 * pi / 12.0;
    } else {
        return 2 * pi / SIGNAL_RES / getSizes().x * global.pn_scanline_dur * global.pn_color_freq;
    }
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

//////////////////////////////////
// Common functions for filters //
//////////////////////////////////

float sinc(float x) {
    if(x == 0.0)
        return 1.0;
    else
        return sin(x) / x;
}

// Hann window, or a Hamming window with a0 = 0.5
// https://en.wikipedia.org/wiki/Window_function
float hamming(int samplePos, int width) {
    return pow(cos(pi * samplePos / width / 2), 2);
}

// Blackman window
// https://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch16.pdf
float blackman(int samplePos, int width) {
    return 0.42 - 0.5 * cos(2 * pi * samplePos / width) + 0.08 * cos(4 * pi * samplePos / width);
}

/////////////////////////////////
// Three band equalizer filter //
/////////////////////////////////

// Three band equalizer filter, directly ported from NTSC-CRT by EMMIR.
// It worked well in NTSC-CRT because that was written in plain C,
// but it is terrible in a shader.
/* f_lo_mhz - low cutoff frequency (MHz)
 * f_hi_mhz - high cutoff frequency (MHz)
 * g_lo, g_mid, g_hi - gains
 */
vec4 filterEQF(float f_lo_mhz, float f_hi_mhz,
                float g_lo, float g_mid, float g_hi,
                int rgbIndex, float maxDistFailsafe, float overallOffset) {

// https://www.nesdev.org/wiki/NTSC_video as of August 15, 2024
// Each NES pixel is 186 nanoseconds.
#define L_FREQ (global.pn_nes_enable > 0.5 ? 8 / 0.186 : SIGNAL_RES * getSizes().x / global.pn_scanline_dur)
#define CRT_HRES (getSizes().x * SIGNAL_RES)
#define MHz2L(MHz) (CRT_HRES * ((MHz)) / L_FREQ)

#define HISTLEN     7
#define HISTOLD     (HISTLEN - 1) /* oldest entry */
#define HISTNEW     0             /* newest entry */
    
    // struct EQF
    float lf, hf; /* fractions */
    float g[3]; /* gains */
    float fL[4];
    float fH[4];
    float h[HISTLEN]; /* history */
    
//  init_eq(struct EQF *f,
//          int f_lo, int f_hi, int rate,
//          int g_lo, int g_mid, int g_hi)

    float rate = CRT_HRES;
    float f_lo = MHz2L(f_lo_mhz);
    float f_hi = MHz2L(f_hi_mhz);
    
    g[0] = g_lo;
    g[1] = g_mid;
    g[2] = g_hi;
    
    lf = 2 * sin(pi * f_lo / rate);
    hf = 2 * sin(pi * f_hi / rate);
    
    // reset_eq(struct EQF *f)
    
    for(int i = 0; i < HISTLEN; i++)
        h[i] = 0;
    for(int i = 0; i < 4; i++)
        fL[i] = fH[i] = 0;
    
    float r[3];
    
    int maxDistI = int(0.5 + maxDistFailsafe / 100.0 * SIGNAL_RES * getSizes().x);
    
    overallOffset /= SIGNAL_RES;
    
    for(int offset = -maxDistI; offset <= 0; offset++) {
        float s = texture(Source, vTexCoord + vec2((offset - overallOffset) * params.OutputSize.z, 0))[rgbIndex];
        
        // eqf(struct EQF *f, int s)
        fL[0] += lf * (s - fL[0]);
        fH[0] += lf * (s - fH[0]);
        
        for(int i = 1; i < 4; i++) {
            fL[i] += lf * (fL[i - 1] - fL[i]);
            fH[i] += lf * (fH[i - 1] - fH[i]);
        }
        
        r[0] = fL[3];
        r[1] = fH[3] - fL[3];
        r[2] = h[HISTOLD] - fH[3];
        
        for(int i = 0; i < 3; i++) {
            r[i] = r[i] * g[i];
        }
        
        for(int i = HISTOLD; i > 0; i--) {
            h[i] = h[i - 1];
        }
        
        h[HISTNEW] = s;
    }
    
    return vec4(r[0] + r[1] + r[2]);

#undef L_FREQ
#undef CRT_HRES
#undef kHz2L

#undef HISTLEN
#undef HISTOLD
#undef HISTNEW
}

///////////////////////
// Lowpass functions //
///////////////////////

// https://www.mathworks.com/help/signal/ug/fir-filter-design.html as of August 9, 2024
// Lowpass using the "Windowing Method" as described on that website.
// I took this idea from artifact-colors, which has a similar lowpass.
vec4 lowpass(float halfWidth, float level) {
    float freqFactor = getFreqFactor();
    float levelFixed = level * freqFactor;
    int halfWidthInt = int(floor(halfWidth * SIGNAL_RES_CORRECTION + 0.5)); // Round to nearest integer.
    
    float totalMax = 0.;
    vec4 lowPassed = vec4(0);
    for(int i = -halfWidthInt; i <= halfWidthInt; i++) {
        vec4 samp = texture(Source, vTexCoord + vec2(i * params.OutputSize.z, 0));
        float factor = levelFixed * sinc(levelFixed * i) * blackman(i + halfWidthInt, halfWidthInt * 2);
        totalMax += factor;
        lowPassed += factor * samp;
    }
    
    return lowPassed / totalMax;
}

vec4 lowpassGaussian(float sigma, float radius) {
    // Copied and pasted from Guest.r's gaussian blur shader
    
    float SIGMA_H = sigma * SIGNAL_RES_CORRECTION;
    float SIZEH = radius * SIGNAL_RES_CORRECTION; // 2.6 standard deviations = 99%
    float invsqrsigma = 1.0/(2.0*SIGMA_H*SIGMA_H);
    
    float f = fract(params.OutputSize.x * vTexCoord.x);
    f = 0.5 - f;
    vec2 tex = floor(params.OutputSize.xy * vTexCoord)*params.OutputSize.zw + 0.5*params.OutputSize.zw;
    vec4 color = vec4(0.0);
    vec2 dx  = vec2(params.OutputSize.z, 0.0);
    vec4 baseTex = texture(Source, tex);    

    float w;
    float wsum = 0.0;
    vec4 pixel;
    float n = -SIZEH;

    do
    {
	    pixel  = texture(Source, tex + n*dx);
	    w      = exp(-pow(n+f, 2)*invsqrsigma);
	    color  = color + w * pixel;
	    wsum   = wsum + w;
	    n = n + 1.0;
	    
    } while (n <= SIZEH);

    color = color / wsum;
    
    return color;
}

float d(float x, float b)
{
    return (pi*b*min(abs(x)+0.5,1.0/b));
}

float e(float x, float b)
{
    return (pi*b*min(max(abs(x)-0.5,-1.0/b),1.0/b));
}

vec4 lowpassGTU(float signalRes) {
    // Taken from GTU-Famicom by aliaspider
    
    float   offset  = fract((vTexCoord.x * params.OutputSize.x) - 0.5);
    float range = ceil(0.5 + (global.pn_nes_enable < 0.5 ? params.OutputSize.x * SETTINGS_CORRECTION : getSizes().x * SIGNAL_RES) / signalRes);
    float   Y = signalRes * (global.pn_nes_enable < 0.5 ? params.OutputSize.z / SETTINGS_CORRECTION : 1.0 / getSizes().x / SIGNAL_RES); // Remember to compensate for cropped overscan. The user might or might not be cropping it.
    float X;
    vec4 c;
    vec4 combined = vec4(0);

    float i;
//  for (i=-range;i<range+2.0;i++){
    for (i = 1.0 - range; i < range + 1.0; i += 1)
    {
        X = offset - i;
        c = texture(Source, vec2(vTexCoord.x - X * params.OutputSize.z, vTexCoord.y));
        c *= ((d(X, Y) + sin(d(X, Y)) - e(X, Y) - sin(e(X, Y))) / (2.0 * pi));
        combined += c;
    }
    
    return combined;
}

vec4 twoPointCombLuma() {
    float halfPhase;
    if(global.pn_nes_enable > 0.5) {
        halfPhase = 6 * params.OutputSize.z;
    } else {
        halfPhase = ceil(getSizes().x * SIGNAL_RES / global.pn_scanline_dur / global.pn_color_freq / 2.0) * params.OutputSize.z;
    }
//    vec2 coord = vTexCoord + vec2(floor(halfPhase / params.OutputSize.z / 2) * params.OutputSize.z, 0); // Make sure we step by a multiple of an input pixel. Stepping by a fraction doesn't filter as good.
//    return (texture(Source, coord) + texture(Source, coord - vec2(halfPhase, 0))) / 2;
//    return vec4(0);
    return (texture(Source, vTexCoord + vec2(halfPhase / 2.0, 0)) + texture(Source, vTexCoord - vec2(halfPhase / 2.0, 0))) / 2;
}

vec4 lowpassFullPhase(float width) {
    float wavelength = global.pn_nes_enable < 0.5 ? getSizes().x * SIGNAL_RES / global.pn_scanline_dur / global.pn_color_freq : 12;
    wavelength *= width;
    
    int start = int(ceil(-wavelength / 2.0) + 0.5);
    float end = start + floor(wavelength + 0.5);
    
    vec4 sum = vec4(0);
    vec4 samp;
    int i;
    for(i = start; i < end; i++) {
        samp = texture(Source, vTexCoord + vec2(i * params.OutputSize.z, 0.0));
        sum += samp;
    }
//    samp = texture(Source, vTexCoord + vec2(i * params.OutputSize.z, 0.0));
//    sum += samp * (end - (i - 1));
    
    return sum / (end - start);
}

vec4 twoLineComb(bool chroma) {
    float wavelength = global.pn_nes_enable < 0.5 ? getSizes().x * SIGNAL_RES / global.pn_scanline_dur / global.pn_color_freq : 12;
    
    float thisOffset = mod(getPhase() / 2 / pi, 1.0);
    float prevOffset = mod(getPhase(-1) / 2 / pi + 0.5, 1.0);
    
    float diff = mod(thisOffset - prevOffset + 0.5, 1.0) - 0.5;
    
    vec4 thisLuma = texture(Source, vTexCoord);
    vec4 prevLuma = texture(Source, vTexCoord + params.OutputSize.zw * vec2(diff * wavelength, -0.999));
    
    return (thisLuma + prevLuma * (chroma ? -1 : 1)) / 2;
}

vec4 threeLineComb(bool chroma) {
    float wavelength = global.pn_nes_enable < 0.5 ? getSizes().x * SIGNAL_RES / global.pn_scanline_dur / global.pn_color_freq : 12;
    
    float thisOffset = mod(getPhase() / 2 / pi, 1.0);
    float prevOffset = mod(getPhase(-1) / 2 / pi + 0.5, 1.0);
    float nextOffset = mod(getPhase(1) / 2 / pi + 0.5, 1.0);
    
    float diffPrev = mod(thisOffset - prevOffset + 0.5, 1.0) - 0.5;
    float diffNext = mod(thisOffset - nextOffset + 0.5, 1.0) - 0.5;
    
    vec4 thisLuma = texture(Source, vTexCoord);
    vec4 prevLuma = texture(Source, vTexCoord + params.OutputSize.zw * vec2(diffPrev * wavelength, -0.999));
    vec4 nextLuma = texture(Source, vTexCoord + params.OutputSize.zw * vec2(diffNext * wavelength, 0.999));
    
    return (thisLuma + (prevLuma + nextLuma) / 2.0 * (chroma ? -1 : 1)) / 2;
}

vec4 lowpassPickable(float type,
                     float halfWidth, float level, // artifact-colors sinc window
                     float gaussSigma, float gaussRadius, // guest(r) gaussian blur
                     float signalRes) // aliaspider gtu)
{
    if(type < -0.5) {
        return texture(Source, vTexCoord); // No low pass
    } else if(type < 0.5) {
        return lowpass(halfWidth, level); // sinc window
    } else if(type < 1.5) {
        return lowpassGaussian(gaussSigma, gaussRadius);
    } else if (type < 2.5) {
        return lowpassGTU(signalRes);
        
        // 3 onward are for decoding only
    } else if (type < 3.5) {
        return lowpassFullPhase(1.0);
    } else if (type < 4.5) {
        // This filter is only for the demodulator, so it's at the end.
        // The lowpass filter selection for the modulator only goes up to 3 to prevent
        // this one from being picked for that.
        return twoPointCombLuma();
    } else if(type < 5.5) {
        return twoLineComb(false);
    } else {
        return threeLineComb(false);
    }
}

////////////////////////
// Bandpass functions //
////////////////////////

// https://www.mathworks.com/help/signal/ug/fir-filter-design.html as of August 9, 2024
// This method of bandpassing is also taken from artifact-colors. Uses the "Windowing Method".
vec4 bandpass(float halfWidth, float levelLo, float levelHi) {
    float freqFactor = getFreqFactor();
    float levelFixedLo = (levelLo) * freqFactor;
    float levelFixedHi = (levelHi) * freqFactor;
    int halfWidthInt = int(floor(halfWidth * SIGNAL_RES_CORRECTION + 0.5)); // Round to nearest integer.
    
    float burstRate = getBurstRate();
    
    vec4 bandPassed = vec4(0);
    float maxSum = 0.0;
    for(int i = -halfWidthInt; i <= halfWidthInt; i++) {
        vec4 samp = texture(Source, vTexCoord + vec2(i * params.OutputSize.z, 0));
        float window = blackman(i + halfWidthInt, halfWidthInt * 2);
        float factorLo = levelFixedLo * sinc(levelFixedLo * i);
        float factorHi = levelFixedHi * sinc(levelFixedHi * i);
        float factor = factorHi - factorLo;
        bandPassed += window * factor * samp;
        maxSum += window * factor * cos(burstRate * i);
    }
    
    return bandPassed / maxSum;
}

vec4 twoPointCombChroma() {
    float halfPhase;
    if(global.pn_nes_enable < 0.5) {
        halfPhase = params.OutputSize.z * SIGNAL_RES / 2 / (global.pn_scanline_dur * global.pn_color_freq / getSizes().x);
    } else {
        halfPhase = params.OutputSize.z * 6;
    }
    return (texture(Source, vTexCoord + vec2(halfPhase / 2, 0)) - texture(Source, vTexCoord - vec2(halfPhase / 2, 0))) / 2;
}

vec4 bandpassPickable(float type,
                      float halfWidth, float level, float levelDiff)
{
    if(type < -0.5) {
        return texture(Source, vTexCoord);
    } else if(type < 0.5) {
        return bandpass(halfWidth, level, levelDiff);
        
        // 1 is for decoding only
    } else if(type < 1.5) {
        return twoPointCombChroma();
    } else if(type < 2.5) {
        return twoLineComb(true);
    } else {
        return threeLineComb(true);
    }
}

/////////////////////////
// Chroma demodulation //
/////////////////////////



mat3x2 yuvAxisPoints() {
   float r_off, r_max, g_off, g_max, b_off, b_max;
   
   const float Uupscale = 2.03206187221989;
   const float angFix = -123.0 / 180.0 * 3.14159265359;

   if(global.pn_demodulator_std < 0.5) {
      // Standard BT.470 matrix, used for NTSC and PAL SDTV.
      r_off = (123 - 90) / 180.0 * pi; // 33 deg
      r_max = 1.13983;
      g_off = -1.979; // -113 deg
      g_max = 0.70203;
      b_off = 123 / 180.0 * pi; // 123 deg
      b_max = 2.03211;
  } else if(global.pn_demodulator_std < 1.5) {
      // Later standard BT.709 matrix, used for NTSC and PAL HDTV.
      r_off = (123 - 90) / 180.0 * 3.14159265359;
      r_max = 1.28033;
      g_off = -2.085;
      g_max = 0.43703;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 2.12798;
   } else if(global.pn_demodulator_std < 2.5) {
      // Sony CXA2025AS (JP axis) from official documentation, assuming blue is 33 degrees
      r_off = (123 - 95) / 180.0 * 3.14159265359;
      r_max = 0.78;
      g_off = (123 - 240) / 180.0 * 3.14159265359;
      g_max = 0.3;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pn_demodulator_std < 3.5) {
      // CXA1464AS (JP) from official data sheet
      r_off = (123 - 98) / 180.0 * 3.14159265359;
      r_max = 0.78;
      g_off = (123 - 243) / 180.0 * 3.14159265359;
      g_max = 0.31;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pn_demodulator_std < 4.5) {
      // Panasonic AN5367FB
      r_off = (123 - 104) / 180.0 * 3.14159265359;
      r_max = 0.96;
      g_off = (123 - 235) / 180.0 * 3.14159265359;
      g_max = 0.36;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pn_demodulator_std < 5.5) {
      // Toshiba TA8867BN, from official data sheet
      r_off = (123 - 104) / 180.0 * 3.14159265359;
      r_max = 0.91;
      g_off = (123 - 240) / 180.0 * 3.14159265359;
      g_max = 0.31;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pn_demodulator_std < 6.5) {
      // Toshiba TA8867AN, taken from the official data sheet
      r_off = (123 - 112) / 180.0 * 3.14159265359;
      r_max = 0.84;
      g_off = (123 - 237) / 180.0 * 3.14159265359;
      g_max = 0.33;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pn_demodulator_std < 7.5) {
      // Sony CXA2025AS (US axis mode) from the official data sheet
      r_off = (123 - 112) / 180.0 * 3.14159265359; // 112 deg or 11 deg
      r_max = 0.83;
      g_off = (123 - 252) / 180.0 * 3.14159265359; // 252 deg or -129 deg
      g_max = 0.3;
      b_off = 123 / 180.0 * 3.14159265359; // 0 deg or 123.5 deg
      b_max = 1.0;
   } else if(global.pn_demodulator_std < 8.5) {
      // Sony CXA1465AS from the official data sheet
      r_off = (123 - 114) / 180.0 * 3.14159265359;
      r_max = 0.78;
      g_off = (123 - 255) / 180.0 * 3.14159265359;
      g_max = 0.31;
      b_off = 123 / 180.0 * 3.14159265359;
      b_max = 1;
   } else if(global.pn_demodulator_std < 9.5) {
      // Custom from the user, in the same format as in a data sheet.
      r_off = (123 - global.pn_demodulator_r_off) / 180.0 * pi;
      g_off = (123 - global.pn_demodulator_g_off) / 180.0 * pi;
      b_off = 123.0 / 180.0 * pi;
      r_max = global.pn_demodulator_r_amp * Uupscale;
      g_max = global.pn_demodulator_g_amp * Uupscale;
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

mat3x2 yuvSyncRed(mat3x2 yuvAxisPts) {
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
    mat3 fromRgbStd;

    //    if(global.pn_modulator_std < 0.5) {
        // Rec. 601 YUV matrix
        // This is used in the Genesis/MegaDrive and SNES/SFC
        fromRgbStd = mat3x3(
                    0.299, -0.14713, 0.615,
                    0.587, -0.28886, -0.51499,
                    0.114, 0.436, -0.10001
                 );
    //    } else {
    //        // Rec. 709 YUV matrix
    //        // Don't know any specific consoles using this, but I assume this *eventually* became common.
    //        fromRgbStd = mat3(0.2126, -0.09991, 0.615,
    //                      0.7152, -0.33609, -0.55861,
    //                      0.0722, 0.436, -0.05639);
    //    }
    
    vec3 colorBar = vec3(1, 0, 0); // Red
    vec3 yuvJungle = fromRgb * colorBar;
    vec3 yuvStd = fromRgbStd * colorBar;
    float resid = atan(yuvJungle.b, yuvJungle.g) - atan(yuvStd.b, yuvStd.g);
    
    for(int i = 0; i < 3; i++) {
        yuvAxisPts[i][0] -= resid;
    }
    
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
    
    yuvJungle = fromRgb * colorBar;
    yuvStd = fromRgbStd * colorBar;
    float satJungle = sqrt(pow(yuvJungle.g, 2) + pow(yuvJungle.b, 2));
    float satStd = sqrt(pow(yuvStd.g, 2) + pow(yuvStd.b, 2));
    float prop = satJungle / satStd; // What saturation do we set to make this bar match in saturation?
    
    for(int i = 0; i < 3; i++) {
        yuvAxisPts[i][1] *= prop;
    }
    
    return yuvAxisPts;
}

mat3x2 yuvSyncAvg(mat3x2 yuvAxisPts) {
//   if(global.eztvcol3_cmp_console < 1.5) {
//       // This code is made using the mathematical formulas outlined by Chthon at https://forums.libretro.com/t/dogways-grading-shader-slang/27148/561
//       // The result is an RGB to RGB matrix.
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
    mat3 fromRgbStd;

    //    if(global.pn_modulator_std < 0.5) {
        // Rec. 601 YUV matrix
        // This is used in the Genesis/MegaDrive and SNES/SFC
        fromRgbStd = mat3x3(
                    0.299, -0.14713, 0.615,
                    0.587, -0.28886, -0.51499,
                    0.114, 0.436, -0.10001
                 );
    //    } else {
    //        // Rec. 709 YUV matrix
    //        // Don't know any specific consoles using this, but I assume this *eventually* became common.
    //        fromRgbStd = mat3(0.2126, -0.09991, 0.615,
    //                      0.7152, -0.33609, -0.55861,
    //                      0.0722, 0.436, -0.05639);
    //    }
                           
    // Basic linear regression to match the hue rotation (tint) to standard color bars.
    // Simply average up the offsets, and that's the ideal hue rotation.
    // On US CRTs, many people probably rotated this further to get greener greens, but then, some green would become very cyan, and some reds and pinks would become very orange.
    // Other people on US CRTs, especially on the NES, probably rotated a little in the opposite direction to get browner browns.
    // A problem with this method is that this is not a perceptually uniform color space. I will address that sometime else.

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

    // Old least-squares linear regression to match the saturation to the color bars.
    // We have an equation R = Summation for each of 6 bars: ((This Bar's Saturation in Jungle Chip) * (Saturation Factor) - (This Bar's Standard Saturation))^2
    // Meaning, R is the sum of squared differences in saturations. It's a subtraction, not a ratio.
    // Leaving (Saturation Factor) as a variable and simplifying down, the equation is in the format R = A(Saturation Factor)^2 + B(Saturation Factor) + C.
    // The value of (Saturation Factor) that minimizes R is in the center of the parabola, or -B/(2*A). The value C is never used, so we don't calculate it.

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

    //    toRgb[0] *= 1; // Have to add this line here because some GPU drivers force you to multiply all 3 columns together.
    //    toRgb[1] /= satFix;
    //    toRgb[2] /= satFix;

    // New geometric mean to match the saturation to the color bars.
    // Take the proportion of the change on each color bar.
    // Then, take the geometric mean of those six proportions.
    // This has a better result than the linear regression above because saturation is not linear.
    // A problem with this method is that this is not a perceptually uniform color space. I will address that sometime else.

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

    for(int i = 0; i < 3; i++)
        yuvAxisPts[i][1] *= prop;
    
    return yuvAxisPts;
//   }
}

mat3x2 yuvSyncEyeballed(mat3x2 yuvAxisPts) {
    float hue, sat;
    
    // DISCLAIMER: I do not own CRTs with these chips at the moment.
    // I have set these settings on this same program by eye.
    // Meant to be used with brightness 0.1 and 
    
    if(global.pn_demodulator_std < 1.5) {
        // Standard matrices that are already sync'd
        hue = 0.0;
        sat = 1.0;
    } else if(global.pn_demodulator_std < 2.5) {
        // CXA2025AS JP
        hue = 0.0;
        sat = 0.8;
    } else if(global.pn_demodulator_std < 3.5) {
        // CXA1464AS
        hue = 0.0;
        sat = 0.8;
    } else if(global.pn_demodulator_std < 4.5) {
        // AN5367FB
        // Sync'd against the averaged matrix
        yuvAxisPts = yuvSyncAvg(yuvAxisPts);
        hue = -3.0;
        sat = 0.9;  
    } else if(global.pn_demodulator_std < 5.5) {
        // TA8867BN
        // Sync'd against the averaged matrix
        yuvAxisPts = yuvSyncAvg(yuvAxisPts);
        hue = -5.0;
        sat = 0.9;
    } else if(global.pn_demodulator_std < 6.5) {
        // TA8867AN
        // Sync'd against the averaged matrix
        yuvAxisPts = yuvSyncAvg(yuvAxisPts);
        hue = -5.0;
        sat = 0.9;
    } else if(global.pn_demodulator_std < 7.5) {
        // CXA2025AS US
        hue = -12;
        sat = 0.8;
    } else if(global.pn_demodulator_std < 8.5) {
        // CXA1465AS US
        hue = -12;
        sat = 0.8;
    } else if(global.pn_demodulator_std < 9.5) {
        // Custom. Let the user figure it out.
        hue = 0.0;
        sat = 1.0;
    }
    
    for(int i = 0; i < 3; i++) {
        yuvAxisPts[i][0] += hue / 180.0 * pi;
        yuvAxisPts[i][1] *= sat;
    }

    return yuvAxisPts;
}

mat3x2 yuvAxisPointsSynced() {

    // Do not automatically sync the SMPTE-C and Rec. 709 matrices. Just return the default.
    // Automatic syncing had to be done for jungle chips because their defaults are not listed in their data sheets.

   mat3x2 yuvAxisPts = yuvAxisPoints();
   
    if(global.pn_demodulator_std < 1.5 || global.pn_demodulator_std_defaults < -0.5) {
        return yuvAxisPts;
    } else if(global.pn_demodulator_std_defaults < 0.5) {
        return yuvSyncAvg(yuvAxisPts);
    } else if(global.pn_demodulator_std_defaults < 1.5) {
        return yuvSyncRed(yuvAxisPts);
    } else {
        return yuvSyncEyeballed(yuvAxisPts);
    }
}

mat3 YBmyRmyToRGBMatrix() {
    mat3x2 yuvAxisPts = yuvAxisPointsSynced();
    
    float xr, yr, xg, yg, xb, yb;
    xr = yuvAxisPts[0][1] * cos(yuvAxisPts[0][0]);
    yr = yuvAxisPts[0][1] * sin(yuvAxisPts[0][0]);
    xg = yuvAxisPts[1][1] * cos(yuvAxisPts[1][0]);
    yg = yuvAxisPts[1][1] * sin(yuvAxisPts[1][0]);
    xb = yuvAxisPts[2][1] * cos(yuvAxisPts[2][0]);
    yb = yuvAxisPts[2][1] * sin(yuvAxisPts[2][0]);
    
    // YUV to RGB matrix
    mat3 toRgb = mat3(1,  1,  1,
                      xr, xg, xb,
                      yr, yg, yb);
    
    // YUV to Y B-Y R-Y matrix
    mat3 toYBmyRmy = mat3(1, 0, 0,
                          0, xb, xr,
                          0, yb, yr);
    
    // Y B-Y R-Y to YUV to RGB matrix
    return toRgb * inverse(toYBmyRmy);
}

vec2 uvDemodPickable(float type, float chroma, float phase, float tint) {
    mat3x2 axes = yuvAxisPointsSynced();
    if(type < 0.5 || type > 1.5) {
        return 2 * chroma * vec2(axes[2][1] * sin(phase + axes[2][0] + tint), axes[0][1] * sin(phase + axes[0][0] + tint));
    } else {
        return 2 * chroma * vec2(axes[2][1] * sin(phase + pi / 2 + axes[2][0] + tint), axes[0][1] * sin(phase + pi / 2 + axes[0][0] + tint));
    }
}

//////////////////////////////////////////////////
// NES real hardware video capture lookup table //
//////////////////////////////////////////////////


// The current video capture is of low quality. I am going to redo it soon.
// The problem is that it is captured at only 25% saturation (and 25% contrast), which is extremely low.
// The video capture can be done with a much higher saturation. The goal is to undersaturate to avoid clamping over/under 0.0 or 1.0.

vec3 nesRealCaptureLookup(vec3 yBmyRmy, float saturation) {
    mat3x2 yuvAxisPts = yuvAxisPointsSynced();
    
    float xr, yr, xg, yg, xb, yb;
    xr = yuvAxisPts[0][1] * cos(yuvAxisPts[0][0]);
    yr = yuvAxisPts[0][1] * sin(yuvAxisPts[0][0]);
    xg = yuvAxisPts[1][1] * cos(yuvAxisPts[1][0]);
    yg = yuvAxisPts[1][1] * sin(yuvAxisPts[1][0]);
    xb = yuvAxisPts[2][1] * cos(yuvAxisPts[2][0]);
    yb = yuvAxisPts[2][1] * sin(yuvAxisPts[2][0]);
    
    // YUV to Y B-Y R-Y matrix
    mat3 toYBmyRmy = mat3(1, 0, 0,
                          0, xb, xr,
                          0, yb, yr);
    
    // YUV to RGB matrix
    mat3 toRgb = mat3(1,  1,  1,
                      xr, xg, xb,
                      yr, yg, yb);

    vec3 YIQ = mat3(1, 0, 0, // 30 instead of 33 is not a mistake. The hue offsets below were based on GTU-famicom's output, which has its YIQ off by exactly 3 degrees.
                    0, -sin(30.0 * pi / 180.0), cos(30.0 * pi / 180.0),
                    0, cos(30.0 * pi / 180.0), sin(30.0 * pi / 180.0)) *
               inverse(toYBmyRmy) * yBmyRmy;

    // Grays. I only approximated the three grays in the 00-column and put pure black for 1d. I'll probably get the remaining ones from the 0d column sometime later.
    float unsaturatedLevels[4] = {0.0, (0.616 - 0.312) / (1.100 - 0.312), (0.840 - 0.312) / (1.100 - 0.312), 1.0}; // Calculated from DAC.slang 
    // The two equivalent whites 20 and 30 are only one white in this.
    float whiteRed[] = {0, 0.26171875, 0.41796875, 0.62109375, };
    float whiteGreen[] = {0, 0.19921875, 0.3984375, 0.5, };
    float whiteBlue[] = {0, 0.3203125, 0.5390625, 0.7421875, };
    
    // Colors that aren't gray, i.e. x1 thru xC for x = 0, 1, 2, 3
    float levelSats[4] = {sqrt((16 + 9) / 255.0), sqrt((3 * 16) / 255.0), sqrt((3 * 16 + 2) / 255.0), sqrt(9.0 / 255.0)}; // Distance of cartesian point (I, Q) from origin (0, 0). Screenshotted from shader test output.
    float saturatedLevels[4] = {((0.228 + 0.616) / 2. - 0.312) / (1.100 - 0.312), ((0.312 + 0.840) / 2. - 0.312) / (1.100 - 0.312), ((0.552 + 1.100) / 2. - 0.312) / (1.100 - 0.312), ((0.880 + 1.100) / 2. - 0.312) / (1.100 - 0.312)}; // Calculated from DAC.slang
    float levelHues[12] = {0.2890625, 0.3672419921875, 0.4453125, 0.53125, 0.609375, 0.69140625, 0.76953125, 0.890625, 0.96875, 0.0546875, 0.1328125, 0.20703125}; // Screenshotted from shader test output
    float level0Red[] = {0.06640625, 0.11328125, 0.16015625, 0.30078125, 0.3203125, 0.33203125, 0.26171875, 0.12890625, 0.0, 0.0, 0.0, 0.05078125, };
    float level0Green[] = {0.0546875, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.046875, 0.08203125, 0.109375, 0.08984375, 0.04296875, };
    float level0Blue[] = {0.45703125, 0.5, 0.44140625, 0.328125, 0.0703125, 0.0, 0.0, 0.0, 0.0, 0.0, 0.12109375, 0.328125, };
    float level1Red[] = {0.1640625, 0.16796875, 0.3359375, 0.4453125, 0.5078125, 0.5, 0.4609375, 0.37109375, 0.125, 0.0, 0.0, 0.08203125, };
    float level1Green[] = {0.09375, 0.08203125, 0.0, 0.0, 0.08203125, 0.08984375, 0.14453125, 0.2265625, 0.265625, 0.2890625, 0.2734375, 0.27734375, };
    float level1Blue[] = {0.69921875, 0.76953125, 0.7265625, 0.5859375, 0.328125, 0.0, 0.0, 0.0, 0.0, 0.0, 0.22265625, 0.51171875, };
    float level2Red[] = {0.1328125, 0.265625, 0.3984375, 0.5234375, 0.56640625, 0.625, 0.55859375, 0.52734375, 0.359375, 0.21875, 0.0, 0.125, };
    float level2Green[] = {0.41015625, 0.26953125, 0.1953125, 0.203125, 0.19921875, 0.28125, 0.328125, 0.3828125, 0.37890625, 0.44140625, 0.4296875, 0.47265625, };
    float level2Blue[] = {0.80859375, 0.73046875, 0.6953125, 0.70703125, 0.58203125, 0.3359375, 0.07421875, 0.0, 0.0, 0.1484375, 0.3984375, 0.63671875, };
    float level3Red[] = {0.46875, 0.50390625, 0.5390625, 0.59765625, 0.6328125, 0.57421875, 0.609375, 0.58203125, 0.51953125, 0.4375, 0.41796875, 0.390625, };
    float level3Green[] = {0.51953125, 0.48046875, 0.48046875, 0.4765625, 0.4765625, 0.4375, 0.49609375, 0.48828125, 0.48828125, 0.4765625, 0.4765625, 0.4765625, };
    float level3Blue[] = {0.796875, 0.78125, 0.76953125, 0.8046875, 0.7265625, 0.56640625, 0.5390625, 0.4765625, 0.48046875, 0.52734375, 0.56640625, 0.6015625, };
    
    // The saturation amounts were screenshotted from GTU-famicom, which does not properly bandpass the chroma signal.
    // That results in GTU-famicom's chroma being oversaturated, due to the NES's chroma signal being a square wave instead of a sinusoid.
    // Those incorrect saturation amounts can be fixed by just calculating them properly.
    float lowLevels[4] = {(0.228 - 0.312) / (1.100 - 0.312), 0.0, (0.522 - 0.312) / (1.1 - 0.312), (0.880 - 0.312) / (1.1 - 0.312)};
    float highLevels[4] = {(0.616 - 0.312) / (1.1 - 0.312), (0.84 - 0.312) / (1.1 - 0.312), 1.0, 1.0};
    for(int i = 0; i < 4; i++) {
        float amplitude = (highLevels[i] - lowLevels[i]) / 2;
        levelSats[i] = 0.0;
        for(int j = 0; j < 12; j++) {
            levelSats[i] += 2 * amplitude * pow(sin(j / 12.0 * pi / 2.0), 2.0) / 12.0; // Use a sine wave instead of a square wave, to roughly simulate bandpassing. Result is a slight underestimate.
        }
    }
    
    // Screw all that. Replace that entire eyeballed palette with a direct video capture of the NES's composite video.
    // I originally wrote this code months ago, so pardon the mess. I've carefully looked through this
    // horrible mess of code and made sure it works. The only problem is that emphasis bits aren't implemented.
    // I do not know how I would implement emphasis bits into this lookup, but the result is already close enough for me.
    
    float pal[192] = {
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
        
#define OVERWRITE_COLOR(pal_offset, array_offset, arrays) \
    arrays##Red[array_offset] = (pal[(pal_offset) * 3] - 0.368467) / (0.648117 - 0.368467); \
    arrays##Green[array_offset] = (pal[(pal_offset) * 3 + 1] - 0.368467) / (0.648117 - 0.368467); \
    arrays##Blue[array_offset] = (pal[(pal_offset) * 3 + 2] - 0.368467) / (0.648117 - 0.368467);

#define OVERWRITE_ROW(rowNumber) \
    for(int i = 1; i <= 12; i++) { \
        OVERWRITE_COLOR(i + (rowNumber) * 16, (i-1) % 12, level##rowNumber); \
    }

#define OVERWRITE_PALETTE  \
    OVERWRITE_ROW(0); \
    OVERWRITE_ROW(1); \
    OVERWRITE_ROW(2); \
    OVERWRITE_ROW(3); \
    OVERWRITE_COLOR(30, 0, white); \
    OVERWRITE_COLOR(0, 1, white); \
    OVERWRITE_COLOR(16, 2, white); \
    OVERWRITE_COLOR(48, 3, white);
    
    OVERWRITE_PALETTE;
    
    int unsatLevelI = -1; // Integer part. Can be 0, 1, or 2. If the level is 3 or higher, it is represented as 2 with a fractional part greater than 1.
    float unsatLevelF; // Fractional part. If I=0, -1<F<1. If I=1, 0<F<1. If I=2, 0<F<2.
    do {
        if(unsatLevelI == -1)
            unsatLevelI = 0;
        else
            unsatLevelI++; // This if-statement is stupid, but my stupid computer requires me to do this...
        float unsatYRemaining = YIQ.r - unsaturatedLevels[unsatLevelI];
        unsatLevelF = unsatYRemaining / (unsaturatedLevels[unsatLevelI + 1] - unsaturatedLevels[unsatLevelI]);
    } while(unsatLevelI < 3 && unsatLevelF > 1.0);

    // Note that the color does not cap at 0 and 1; it can be negative or greater than 1 too.
    vec3 unsaturatedColor = vec3(whiteRed[unsatLevelI], whiteGreen[unsatLevelI], whiteBlue[unsatLevelI]) * (1 - unsatLevelF) +
                            unsatLevelF * vec3(whiteRed[unsatLevelI + 1], whiteGreen[unsatLevelI + 1], whiteBlue[unsatLevelI + 1]);

    int satLevelI = -1; // Integer part. Can be 0, 1, or 2. If the level is 3 or higher, it is represented as 2 with a fractional part greater than 1.
    float satLevelF; // Fractional part. If I=0, -1<F<1. If I=1, 0<F<1. If I=2, 0<F<2.
    do {
        if(satLevelI < 0)
            satLevelI = 0;
        else
            satLevelI++;
        float satYRemaining = YIQ.r - saturatedLevels[satLevelI];
        satLevelF = satYRemaining / (saturatedLevels[satLevelI + 1] - saturatedLevels[satLevelI]);
    } while(satLevelI < 2 && satLevelF > 1.0);
    
    float hue = atan(YIQ.g, YIQ.b) / 2 / pi + 0.5;
    int floorI = -1; // Hue integer that's less than hue.
    int ceilI = 0; // Hue integer that's greater than hue.
    float partI; // Fractional part of hue.
    
    // Performed a linear regression on screenshotted test output.
    // As long as the preset's settings are all nearest-neighbor at 8x horizontal resolution,
    // this is the same for all setups.
    float hueFitted = (hue - 0.0418747) / 0.0826453 + 12.0 - 3.0;
    floorI = int(floor(hueFitted) + 0.5);
    ceilI = floorI + 1;
    partI = hueFitted - floorI;
    while(floorI >= 12)
        floorI -= 12;
    while(ceilI >= 12)
        ceilI -= 12;
    
    // Old code using an array of screenshotted values.
//    do {
//        floorI++;
//        ceilI = (floorI + 1) % 12;
//        float floorH = levelHues[floorI];
//        float ceilH = levelHues[ceilI];
//        if(floorH < ceilH) {
//            partI = (hue - floorH) / (ceilH - floorH);
//        } else if(hue <= ceilH) {
//            partI = (hue + 1 - floorH) / (ceilH + 1 - floorH);
//        } else if(hue >= floorH) {
//            partI = (hue - floorH) / (ceilH + 1 - floorH);
//        } else { // hue >= ceilH && hue <= floorH
//            continue;
//        }
//    } while(floorI < 12 && (partI >= 1.0 || partI < 0.0));

    vec3 floorLow, ceilLow, floorHigh, ceilHigh;
    switch(satLevelI) {
    case 0:
        floorLow = vec3(level0Red[floorI], level0Green[floorI], level0Blue[floorI]);
        ceilLow = vec3(level0Red[ceilI], level0Green[ceilI], level0Blue[ceilI]);
        floorHigh = vec3(level1Red[floorI], level1Green[floorI], level1Blue[floorI]);
        ceilHigh = vec3(level1Red[ceilI], level1Green[ceilI], level1Blue[ceilI]);

//        vec3 floorDiff = floorHigh - floorLow;
//        vec3 ceilDiff = ceilHigh - ceilLow;
//        while(satLevelF < 0) {
//            satLevelF++;
//            floorLow -= floorDiff;
//            floorHigh -= floorDiff;
//            ceilLow -= ceilDiff;
//            ceilHigh -= ceilDiff;
//        }
        break;
    case 1:
        floorLow = vec3(level1Red[floorI], level1Green[floorI], level1Blue[floorI]);
        ceilLow = vec3(level1Red[ceilI], level1Green[ceilI], level1Blue[ceilI]);
        floorHigh = vec3(level2Red[floorI], level2Green[floorI], level2Blue[floorI]);
        ceilHigh = vec3(level2Red[ceilI], level2Green[ceilI], level2Blue[ceilI]);
        break;
    case 2:
        floorLow = vec3(level2Red[floorI], level2Green[floorI], level2Blue[floorI]);
        ceilLow = vec3(level2Red[ceilI], level2Green[ceilI], level2Blue[ceilI]);
        floorHigh = vec3(level3Red[floorI], level3Green[floorI], level3Blue[floorI]);
        ceilHigh = vec3(level3Red[ceilI], level3Green[ceilI], level3Blue[ceilI]);
        
        break;
    }
    
    // I've since learned that each of these calculations can easily be replaced with mix().
    vec3 saturatedLow = floorLow + partI * (ceilLow - floorLow);
    vec3 saturatedHigh = floorHigh + partI * (ceilHigh - floorHigh);
    
    vec3 saturatedColor;
    float expectedSaturationRaw;
    
    if(satLevelI == 0 && satLevelF < 0.0) {
        // For some palettes, it might be better to use the normal "else" clause instead of this special case.
        expectedSaturationRaw = levelSats[0];
        saturatedColor = saturatedLow + (YIQ.r - saturatedLevels[0]) / (unsaturatedLevels[1] - unsaturatedLevels[0]) * (vec3(whiteRed[1] - whiteRed[0], whiteGreen[1] - whiteGreen[0], whiteBlue[1] - whiteBlue[0]));
    } else if(satLevelI == 2 && satLevelF > 1.0) {
        expectedSaturationRaw = levelSats[3];
        saturatedColor = saturatedHigh + (YIQ.r - saturatedLevels[3]) / (unsaturatedLevels[3] - unsaturatedLevels[2]) * (vec3(whiteRed[3] - whiteRed[2], whiteGreen[3] - whiteGreen[2], whiteBlue[3] - whiteBlue[2]));
    } else {
        saturatedColor = saturatedLow + satLevelF * (saturatedHigh - saturatedLow); // saturatedLow * (1 - satLevelF) + saturatedHigh * satLevelF;
        expectedSaturationRaw = levelSats[satLevelI] + satLevelF * (levelSats[satLevelI + 1] - levelSats[satLevelI]);
    }
    
    float saturationRaw = sqrt(YIQ.g * YIQ.g + YIQ.b * YIQ.b);
    float saturationNorm = saturationRaw / expectedSaturationRaw;
    //RGB = vec3(saturationNorm);
    vec3 RGB = unsaturatedColor + saturationNorm * (saturatedColor - unsaturatedColor);
    
    return toRgb *
           mat3(1, 0, 0,
                0, saturation, 0,
                0, 0, saturation) *
           mat3(0.2126, -0.09991, 0.615, // Rec. 709 YUV matrix.
                0.7152, -0.33609, -0.55861, // My video capture used Rec. 709.
                0.0722, 0.436, -0.05639) * RGB;
}

vec2 uvFilterPickable(float type) {
    if(type < -0.5) {
        return texture(Source, vTexCoord).gb;
    } else {
        return lowpassFullPhase(global.pn_demodulator_uv_filter_width).gb;
    }
}

//////////////////
// Inverse EOTF //
//////////////////

// CRT EOTF function taken from Grade; modified to perform its steps backward.
#define CRT_l           -(100000.*log((72981.-500000./(3.*max(2.3,global.pn_g_CRT_l)))/9058.))/945461.
float EOTF_1886a_inverse_max(float bl, float brightness, float contrast) {
    
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
    
    // What input value results in 1.0?
    float bc = 0.00446395*pow(bl,1.23486);
    float color = 1.0;
    color = pow(color, 1.0 / (1.0-0.00843283*pow(bl,1.22744)));
    color = color / (1.0 / (1.0 - bc)) + bc;
    
    color = pow(color / k, 1.0 / a1) - Lb;

    return color;
}

float EOTF_1886a_default_inverse_max(float brightness, float contrast) {
    // From Rec. ITU-R BT.1886
    float Vc = 0.35, // Vc, a1, and a2 are directly from the paper
          a1 = 2.6,
          a2 = 3.0,
          Lw = contrast;
    
    float V = 1.0,
          b = brightness, // The paper recommends about either 0 or 0.1.
          k = Lw / pow(1.0 + b, a1);
    
    // What value of V gives a result of 1.0?
//    1.0 = k * pow(V + b, a1)
//    pow(1.0 / k, 1.0 / a1) - b = V

    return pow(1.0 / k, 1.0 / a1) - b;
}

float gammaInverseMaxPickable() {
    if(global.pn_gamma_type < 0.5) {
        return 1.0 / EOTF_1886a_inverse_max(CRT_l, global.pn_knob_brightness, global.pn_knob_contrast);
    } else if(global.pn_gamma_type < 1.5) {
        return 1.0 / EOTF_1886a_default_inverse_max(global.pn_knob_brightness, global.pn_knob_contrast);
    } else if(global.pn_gamma_type < 2.5) {
        return pow(global.pn_knob_contrast, 1.0 / global.pn_power_gamma);
    } else {
        float contr = 1.055 * pow(global.pn_knob_contrast, 1.0 / 2.4) - 0.055; // 2.4 instead of 2.2 is not a mistake.
//        rgb = (rgb + global.pn_knob_brightness) / (1 + global.pn_knob_brightness) * contr;
//        rgb * (1 + global.pn_knob_brightness) / contr - global.pn_knob_brightness = rgb
        return 1.0 / ((1 + global.pn_knob_brightness) / contr - global.pn_knob_brightness);
    }
}

#endif // __patchy_ntsc_inc_filters_inc__


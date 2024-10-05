#version 450

/*
Patchy NTSC
Copyright (C) 2024 Patchy68k/PlainOldPants

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 

*/

// This pass separates luma and chroma from each other and demodulates chroma into B-Y and R-Y, using B-Y and R-Y offsets and multipliers taken from real jungle chips, taking into account the Tint knob/OSD setting.

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

// Includes lowpass and bandpass functions
#include "patchy-ntsc-inc-filters.inc"

void main()
{
    if(global.pn_connection_type > 1.5) { // No Artifacts
        
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
        
        FragColor = vec4(toYBmyRmy * texture(Source, vTexCoord).rgb, 1.0);
    } else {
        float luma = 0.0;
        float chroma = 0.0;
        
        float phase = getPhase();
        
        if(global.pn_connection_type < 0.5) { // Composite
            // Filter the signal to isolate luma
            luma += lowpassPickable(global.pn_demodulator_luma_filter_type,
                global.pn_demodulator_luma_filter_width, global.pn_demodulator_luma_filter_level,
                global.pn_demodulator_luma_radius, global.pn_demodulator_luma_sigma,
                global.pn_demodulator_luma_res).r;
            // Filter the signal to isolate chroma
            chroma += bandpassPickable(global.pn_demodulator_chroma_filter_type,
                global.pn_demodulator_chroma_filter_width, global.pn_demodulator_chroma_filter_level, global.pn_demodulator_chroma_filter_level_diff).r;
        } else {
            // S-Video
            vec2 sig = texture(Source, vTexCoord).rg;
            luma = sig.g;
            chroma = sig.r;
        }
        
        if(global.pn_nes_enable < 0.5) { // Composite & S-Video
            luma *= global.pn_color_amplitude; // Correction by the color carrier amplitude
        }
        
        float tint = global.pn_knob_tint * pi / 180.0;
        
        if(global.pn_nes_enable > 0.5 && global.pn_nes_real_capture < 0.5) {
            // From lidnariq's measurements at http://forums.nesdev.org/viewtopic.php?p=159266#p159266
            // 30 = colorburst max
            // -23 = colorburst min
            // 110 = white
            // 0 = black
            float nesAmp = (30.0 - -23.0) / 2.0 / (110.0 - 0.0);
            float stdAmp = 0.2 * 100.0 / 110.0; // Standard colorburst amplitude in NTSC
            chroma *= stdAmp / nesAmp;
            float saturatedLevels[4] = {((0.228 + 0.616) / 2. - 0.312) / (1.100 - 0.312), ((0.312 + 0.840) / 2. - 0.312) / (1.100 - 0.312), ((0.552 + 1.100) / 2. - 0.312) / (1.100 - 0.312), ((0.880 + 1.100) / 2. - 0.312) / (1.100 - 0.312)}; // Calculated from DAC.slang from GTU-famicom
            
            int satLevelI = -1; // Integer part. Can be 0, 1, or 2. If the level is 3 or higher, it is represented as 2 with a fractional part greater than 1.
            float satLevelF; // Fractional part. If I=0, -1<F<1. If I=1, 0<F<1. If I=2, 0<F<2.
            do {
                if(satLevelI < 0)
                    satLevelI = 0;
                else
                    satLevelI++;
                float satYRemaining = luma - saturatedLevels[satLevelI];
                satLevelF = satYRemaining / (saturatedLevels[satLevelI + 1] - saturatedLevels[satLevelI]);
            } while(satLevelI < 2 && satLevelF > 1.0);
            
            // Skew amounts taken from Drag's NES palette generator.
            // I remember seeing these on the nesdev forums somewhere too.
            // The change is done here instead of in the video signal for better precision.
            float skew;
            if(global.pn_nes_hue_skew < 0.5) {
                // 2C02G
                skew = -5.0 / 180.0 * pi; // -5 degrees
            } else {
                // 2C02E
                skew = -2.5 / 180.0 * pi; // -2.5 degrees
            }
            
            tint += (3.0 - (satLevelI + satLevelF)) * skew;
        }
        
        vec2 bmyRmy = uvDemodPickable(global.pn_demodulator_chroma_filter_type, chroma, phase, tint);
        
        FragColor = vec4(luma, bmyRmy, 1);
    }
}


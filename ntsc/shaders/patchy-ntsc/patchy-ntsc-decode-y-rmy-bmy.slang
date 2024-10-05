#version 450

/*
Patchy NTSC
Copyright (C) 2024 Patchy68k/PlainOldPants

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 

*/

// This pass lowpasses R-Y and B-Y and combines the result with Y to get RGB with corrections taken from jungle chips that actually existed, taking the knobs/on-screen-display settings into account.

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
    float y = texture(Source, vTexCoord).r;
    vec2 rmybmy;
    
    if(global.pn_connection_type < 1.5) { // Composite and S-Video
        // A slight lowpass on B-Y and R-Y after demodulating
        rmybmy = uvFilterPickable(global.pn_demodulator_uv_filter_type);
    } else {
        // No Artifacts
        rmybmy = texture(Source, vTexCoord).gb;
    }
    
    vec3 rgb;
    
    if(global.pn_nes_enable > 0.5 && global.pn_nes_real_capture > 0.5) {
        rgb = nesRealCaptureLookup(vec3(y, rmybmy), global.pn_knob_saturation);
    } else {
        rgb = YBmyRmyToRGBMatrix() * vec3(y, rmybmy * global.pn_knob_saturation);
    }
    
    rgb = (rgb + global.pn_knob_brightness) / (1 + global.pn_knob_brightness);
    rgb *= gammaInverseMaxPickable();
    
    if(global.pn_rgb_smear_enable > 0.5)
        rgb = min(vec3(global.pn_rgb_smear_clamp), rgb);
    
    FragColor = vec4(rgb, 1);
//    FragColor = vec4(y, u, v, 1);
}


#version 450

/*
Patchy NTSC
Copyright (C) 2024 Patchy68k/PlainOldPants

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 

*/

// This pass filters the luma signal and adds the chroma signal to get the composite signal.

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

float getNoise() {
    // Compensate for cropped overscan, assuming equal crop on both sides.
    // Current position in pixels, both x and y being decimal numbers.
    vec2 pixelCoord = realPixelCoord();
    float noise = 0.0;
    
    // Add RF noise
    
    float offsetXPart = pixelCoord.x * 47.7 / global.pn_scanline_dur * 256.0 / 341.25;
    int offsetYPart = int(pixelCoord.y + 0.5);
    int offsetFramePart = int(params.FrameCount + 0.5);
    float offsetFull = offsetXPart + offsetYPart + 262 * offsetFramePart;
    
    // https://en.wikipedia.org/wiki/Linear_congruential_generator
    const uint modulus = 0x7fffffff; // Use & instead of % for better performance
    const uint multiplier = 48271;
    const uint increment = 0;
    const uint truncation = 8;
    uint randOffset = int(global.pn_noise_rand_offset / 1000.0 * modulus + 0.5);
#define RNG (((randOffset = (randOffset * multiplier + increment) & modulus) >> truncation) / float(modulus >> truncation))

    float minRate = global.pn_noise_min_rate;
    float maxRate = global.pn_noise_max_rate;

    for(int i = 0; i < int(global.pn_noise_counter + 0.5); i++) {
        float rate = RNG * (maxRate - minRate) + minRate;
        
        // WRONG way to do this part. If you do this, the float that we pass into the sin() function will get very big, and our noise will start disappearing over time and suddenly reappearing in a 1000-frame cycle.
//        noise += (randoms[(i * 2 + randOffset) % 1000] * (1.0 - minAmp) + minAmp) * sin(offsetFull * rate);
        
        // New way to avoid getting too large of a number in the sin() function.
//        float overAmt = mod(rate, 2 * pi);
        float overAmt = rate;
        float overAmtFrame = overAmt * 262 - floor(overAmt * (262 / (2 * pi))) * (2 * pi);
//        float overAmtFrame = overAmt * 262;
        
        noise += sin(offsetXPart * rate + offsetYPart * overAmt + offsetFramePart * overAmtFrame) * global.pn_noise_severity / global.pn_noise_counter;
    }
    
#undef RNG
    
    return noise;
}

void main()
{
    vec4 orig = texture(Source, vTexCoord);
    if(global.pn_connection_type < -0.5)
        orig.r += getNoise();
    
    FragColor = orig;
}


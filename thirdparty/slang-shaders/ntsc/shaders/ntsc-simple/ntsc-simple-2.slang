#version 450
/*
   Simple Composite-like shader by DariusG 2023/24
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.


*/

layout(push_constant) uniform Push
{
  float system_choose, bogus, steps, ntsc_sharp, ntsc_sat, anim_overr;
} params;
#pragma parameter anim_overr "Force Animate Artifacts" 0.0 0.0 1.0 1.0
#pragma parameter system_choose "SYSTEM CHOOSE" 0.0 0.0 6.0 1.0
#pragma parameter bogus "[ 0:NINTENDO 1:GEN 2:PCE 3:MS-MD256 4:ZXSp 5:C=64 6:A26]" 0.0 0.0 1.0 1.0
#pragma parameter steps "Filter Size (faster)" 4.0 1.0 16.0 1.0
#pragma parameter ntsc_sharp "NTSC Sharpness" 0.1 0.0 1.0 0.01
#pragma parameter ntsc_sat "NTSC Saturation" 2.0 0.0 4.0 0.05

#define system_choose params.system_choose
#define bogus params.bogus
#define steps params.steps
#define ntsc_sharp params.ntsc_sharp
#define ntsc_sat params.ntsc_sat
#define anim_overr params.anim_overr

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;

} global;


#define OutputSize global.OutputSize
#define SourceSize global.SourceSize
#define OriginalSize global.OriginalSize
#define FrameCount global.FrameCount


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

mat3 rgb2yuv = mat3(0.299, 0.587, 0.114,
                        -0.299, -0.587, 0.886, 
                         0.701, -0.587, -0.114);
mat3 yuv2rgb = mat3(1.0, 0.0, 1.13983,
                          1.0, -0.39465, -0.58060,
                          1.0, 2.03211, 0.0);

#define pi 3.1415926
#define NTSC_CLOCK 3.579545
#define PAL_CLOCK 4.43361

void main()
{
    // nes & snes master clock, ppu needs 4 cycles per pixel, pce too on 256px. 320px pce is 21.47/3.0
    // ms too is the same clock
    float system_clock = 21.47727273/4.0; 
    if (system_choose == 1.0) system_clock = NTSC_CLOCK/(15.0*NTSC_CLOCK/8.0);
    // ZX Spectrum PAL clock
    if (system_choose == 4.0) system_clock = PAL_CLOCK/7.0/2.0;
    // c64 runs 4*PAL/NTSC then divides 18 or 14(ntsc), feeds the cpu freq, then runs 8x times of cpu
    if (system_choose == 5.0) system_clock = PAL_CLOCK/(PAL_CLOCK*4.0/18.0*8.0);
    // Atari 2600 is 1:1 ntsc
    if (system_choose == 6.0) system_clock = 3.579545*2.0; // stella outputs double pixels?

    float phase_alt = NTSC_CLOCK/system_clock;
    float v_phase_alt = phase_alt;
    float timer = mod(float(FrameCount/2),2.0);
    
    // md doesn't alternate every line, doesn't animate too
    if (system_choose == 1.0) {v_phase_alt =0.0; timer = 0.0;}
    // pce alternates every two lines
    if (system_choose == 2.0) {v_phase_alt = 1.0; timer = 0.0;}
    if (system_choose == 3.0) {v_phase_alt = 0.0; timer = 0.0;}
    float altv = 0.0;
    if (system_choose == 4.0 || system_choose == 5.0) {v_phase_alt = 0.0; timer = 0.0; 
        altv = mod(floor(vTexCoord.y * SourceSize.y + 0.5), 2.0) * pi;}
    if (system_choose == 5.0 || system_choose == 6.0) {v_phase_alt = 0.0; timer = 0.0; 
        altv = mod(floor(vTexCoord.y * SourceSize.y + 0.5), 2.0) * pi;}
    if (system_choose == 6.0) {v_phase_alt = 0.0; timer = 0.0; }

    if (anim_overr == 1.0) timer = mod(float(FrameCount/2),2.0);    
      
    vec3 res = vec3(0.0);
    float sum = 0.0;
    vec2 ps = vec2(SourceSize.z,0.0);
    int N = int(steps);
for (int i=-N; i<N; i++)
    {

    float n = float(i);
    float w = exp(-ntsc_sharp*n*n);
    float phase = (vTexCoord.x*SourceSize.x + n)*pi*phase_alt - vTexCoord.y*SourceSize.y*pi*v_phase_alt + timer*pi*phase_alt + altv;
    vec3 carrier = vec3(1.0,ntsc_sat*cos(phase),ntsc_sat*sin(phase));
    res += w*texture(Source, vTexCoord + ps*n).rgb*carrier;
    sum += w;
    }
    res /= sum;
    FragColor.rgb = res * yuv2rgb;
} 

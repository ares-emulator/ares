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
  float ntsc_sat, afacts, phase_shifti, phase_shiftq, iq_width, y_width;
} params;

#pragma parameter ntsc_sat "NTSC Saturation" 2.5 0.0 6.0 0.05
#pragma parameter iq_width "Chroma Width (Bleed)" 8.0 4.0 32.0 2.0
#pragma parameter y_width "Luma Width (Blurry)" 2.0 1.0 8.0 1.0
#pragma parameter afacts "NTSC Artifacts Strength (lowpass Y)" 0.02 0.0 1.0 0.01
#pragma parameter phase_shifti "Phase Shift I" 0.0 -5.0 5.0 0.05
#pragma parameter phase_shiftq "Phase Shift Q" 0.0 -5.0 5.0 0.05

#define ntsc_sat params.ntsc_sat
#define iq_width params.iq_width
#define y_width params.y_width
#define afacts params.afacts
#define phase_shifti params.phase_shifti
#define phase_shiftq params.phase_shiftq

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


#define TAU  6.28318530717958647693
#define PI 3.1415926


const mat3 YUV2RGB = mat3(1.0, 0.0, 1.13983,
                          1.0, -0.39465, -0.58060,
                          1.0, 2.03211, 0.0);

float hann(float i, int size, float phase) {
    return pow(sin((PI * (i + phase)) / float(size)), 2.0);
}

void main()
{
vec2 size = SourceSize.xy;
vec2 uv = vTexCoord;
int a = int(iq_width);
int b = int(y_width);

    //Sample composite signal and decode to YIQ
    vec3 YIQ = vec3(0);
    float sum = 0.0;
    for (int n=-b; n<b; n++) {
        // lowpass
        float w = exp(-afacts*float(n)*float(n));
        vec2 pos = uv + vec2(float(n) / size.x, 0.0);
        // low pass Y signal, high frequency chroma pattern is cut-off
        YIQ.x += texture(Source, pos).r*w ;
        sum += w;
        }
        YIQ.x /= sum;

    for (int n=-a; n<a; n++) {
        vec2 pos = uv + vec2(float(n) / size.x, 0.0);
        float phase = (vTexCoord.x*SourceSize.x + float(n))*PI*0.5 -mod(vTexCoord.y*SourceSize.y,2.0)*PI ;

    // add hann window function
        YIQ.yz += texture(Source, pos).gb * ntsc_sat*4.0*vec2(sin(phase+phase_shifti), cos(phase+phase_shiftq))*hann(float(n),a*4,0.0);
        }
    YIQ.yz /= iq_width*2.0;

    //  Convert YIQ signal to RGB
    YIQ = YIQ*YUV2RGB;
    FragColor = vec4(YIQ, 1.0);
    
}

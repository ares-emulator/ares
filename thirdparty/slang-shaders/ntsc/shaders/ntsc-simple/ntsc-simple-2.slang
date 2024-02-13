#version 450
/*
   Simple S-video like shader by DariusG 2023
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.


*/

layout(push_constant) uniform Push
{
  float NTSC_bri,NTSC_sat,ARTIFACTING,FRINGING;
} params;


#pragma parameter NTSC_sat "NTSC SATURATION" 1.0 0.0 4.0 0.05
#pragma parameter NTSC_bri "NTSC BRIGHTNESS" 1.0 0.0 2.0 0.05
#pragma parameter FRINGING "NTSC FRINGING" 0.15 0.0 1.0 0.05
#pragma parameter ARTIFACTING "NTSC ARTIFACTING" 0.25 0.0 1.0 0.05


#define NTSC_sat params.NTSC_sat
#define NTSC_bri params.NTSC_bri
#define FRINGING params.FRINGING
#define ARTIFACTING params.ARTIFACTING



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


//  Colorspace conversion matrix for YIQ-to-RGB
const mat3 YIQ2RGB = mat3(1.000, 1.000, 1.000,
                          0.956,-0.272,-1.106,
                          0.621,-0.647, 1.703);


void main()
{
    vec2 size = SourceSize.xy;
    vec2 uv = vTexCoord;

        //  Sample composite signal and decode to YIQ
        vec3 YIQ = vec3(0);
        for (int n=-2; n<2; n++) {
            vec2 pos = uv + vec2(float(n) / size.x, 0.0);
            YIQ.x += texture(Source, pos).r ;
        }
        YIQ.x /= 4.0;

        for (int n=-4; n<4; n++) {
            vec2 pos = uv + vec2(float(n) / size.x, 0.0);
            float phase = (vTexCoord.x * SourceSize.x+ float(n)) * TAU / 4.0+vTexCoord.y*SourceSize.y*2.0*PI/3.0 ;
            YIQ.yz += texture(Source, pos).gb * vec2(cos(phase), sin(phase));
        }
        YIQ.yz /= 8.0;


        //  Convert YIQ signal to RGB
        YIQ = YIQ2RGB*YIQ;

        FragColor = vec4(YIQ, 1.0);
    
}

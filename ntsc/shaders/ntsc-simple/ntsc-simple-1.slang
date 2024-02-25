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
  float NTSC_bri,ntsc_hue;
} params;


#pragma parameter NTSC_bri "NTSC Brightness" 1.1 0.0 2.0 0.05
#pragma parameter ntsc_hue "NTSC Hue" -3.0 -5.0 5.0 0.05


#define NTSC_bri params.NTSC_bri
#define ntsc_hue params.ntsc_hue

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

const mat3 RGBYUV = mat3(0.299, 0.587, 0.114,
                        -0.299, -0.587, 0.886, 
                         0.701, -0.587, -0.114);

void main()
{
    float phase = (vTexCoord.x*SourceSize.x)*PI/2.0 +mod(vTexCoord.y*SourceSize.y,2.0)*PI;
    phase += ntsc_hue;

    vec3 YIQ = texture(Source,vTexCoord).rgb; 
    YIQ = YIQ*RGBYUV; 
    vec3 signal = vec3(YIQ.x*NTSC_bri + 0.5*sin(phase)*YIQ.y + 0.5*YIQ.z*cos(phase) );   
    FragColor = vec4(signal, 1.0);
    
}

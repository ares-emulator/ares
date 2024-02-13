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

const mat3 RGBYIQ = mat3(0.299, 0.596, 0.211,
                             0.587,-0.274,-0.523,
                             0.114,-0.322, 0.312);

void main()
{
    
mat3 mix_mat = mat3(NTSC_bri  , FRINGING      , FRINGING, 
                   ARTIFACTING, 2.0 * NTSC_sat, 0.0, 
                   ARTIFACTING, 0.0           , 2.0 * NTSC_sat);

    float phase = (vTexCoord.x*SourceSize.x) * PI/2.0 +vTexCoord.y*SourceSize.y*2.0*PI/3.0;
    vec3 YIQ = texture(Source,vTexCoord).rgb; 
    YIQ = YIQ*RGBYIQ; 
    vec3 signal = vec3(YIQ.x, cos(phase)*YIQ.y,  YIQ.z*sin(phase) );   
    FragColor = vec4(signal*mix_mat, 1.0);
    
}

#version 450

layout(push_constant) uniform Push
{
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    float signalResolution;
    float addNoise;
    float noiseStrength;
    uint FrameCount;
} params;

#pragma parameter signalResolution "Signal Resolution" 700.0 20.0 2000.0 10.0
#pragma parameter addNoise "Add Noise" 0.0 0.0 1.0 1.0
#pragma parameter noiseStrength "Noise Strength" 0.0 0.0 1.0 0.05

//#include "config.h"

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

////////////////////////////////////////////////////////
//  GTU-famicom version 0.50
//  Author: aliaspider - aliaspider@gmail.com
//  License: GPLv3      
////////////////////////////////////////////////////////

#define pi          3.14159265358
#define a(x) abs(x)
#define d(x,b) (pi*b*min(a(x)+0.5,1.0/b))
#define e(x,b) (pi*b*min(max(a(x)-0.5,-1.0/b),1.0/b))
#define STU(x,b) ((d(x,b)+sin(d(x,b))-e(x,b)-sin(e(x,b)))/(2.0*pi))
#define X(i) (offset-(i))
#define S(i) (texture(Source, vec2(vTexCoord.x - X(i)/params.SourceSize.x,vTexCoord.y)).x)
#define VAL(i) (S(i)*STU(X(i),(params.signalResolution * params.SourceSize.z)))

float rand2(vec2 co)
{
    float c = 43758.5453;
    float dt = dot(co.xy, vec2(12.9898, 78.233));
    float sn = mod(dt, 3.14);
    
    return fract(sin(sn) * c);
}

float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

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

void main()
{
    float   offset  =   fract((vTexCoord.x * params.SourceSize.x) - 0.50001);
    float   signal  =   0.0;
    float   range   =   ceil(0.50001 + params.SourceSize.x / params.signalResolution);
    range           =   min(range, 255.0);
            
    float i;
    for (i = 1 - range; i < 1 + range; i++)
        signal += VAL(i);

    if (params.addNoise > 0.0)
    {
        vec2 pos = (vTexCoord.xy * params.SourceSize.xy);
        signal -= 0.5;
        signal += (rand(vec2(pos.x * pos.y, params.FrameCount)) - 0.50001) * params.noiseStrength;
        signal += 0.5;
    }
    
    FragColor = vec4(signal);
}

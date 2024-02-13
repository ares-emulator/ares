#version 450

layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float cropOverscan_x;
    float signalResolutionY;
    float signalResolutionI;
    float signalResolutionQ;
} params;

#pragma parameter cropOverscan_x "Crop Overscan X" 0.0 0.0 1.0 1.0
#pragma parameter signalResolutionY "Signal Res Y" 200.0 20.0 500.0 10.0
#pragma parameter signalResolutionI "Signal Res I" 125.0 20.0 350.0 10.0
#pragma parameter signalResolutionQ "Signal Res Q" 125.0 20.0 350.0 10.0

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

float d(float x, float b)
{
    return (pi*b*min(abs(x)+0.5,1.0/b));
}

float e(float x, float b)
{
    return (pi*b*min(max(abs(x)-0.5,-1.0/b),1.0/b));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
    gl_Position = global.MVP * Position;
    
    if (params.cropOverscan_x > 0.0)
        gl_Position.x /= (240.0 / 256.0);
    
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    float   offset  = fract((vTexCoord.x * params.SourceSize.x) - 0.5);
    vec3    YIQ =   vec3(0.0);
    vec3    RGB =   vec3(0.0);
    float   X;
    vec3    c;
    float   Y = params.signalResolutionY * params.SourceSize.z;
    float   I = params.signalResolutionI * params.SourceSize.z;
    float   Q = params.signalResolutionQ * params.SourceSize.z;
    
    float range = ceil(0.5 + params.SourceSize.x / min(min(params.signalResolutionY, params.signalResolutionI), params.signalResolutionQ));
    
    float i;
//  for (i=-range;i<range+2.0;i++){
    for (i = 1.0 - range; i < range + 1.0; i++)
    {
        X = offset - i;
        c = texture(Source, vec2(vTexCoord.x - X * params.SourceSize.z, vTexCoord.y)).rgb;
        c.x *= ((d(X, Y) + sin(d(X, Y)) - e(X, Y) - sin(e(X, Y))) / (2.0 * pi));
        c.y *= ((d(X, I) + sin(d(X, I)) - e(X, I) - sin(e(X, I))) / (2.0 * pi));
        c.z *= ((d(X, Q) + sin(d(X, Q)) - e(X, Q) - sin(e(X, Q))) / (2.0 * pi));
        YIQ += c;
    }

    RGB.r = YIQ.r + 0.956 * YIQ.g + 0.621 * YIQ.b;
    RGB.g = YIQ.r - 0.272 * YIQ.g - 0.647 * YIQ.b;
    RGB.b = YIQ.r - 1.106 * YIQ.g + 1.703 * YIQ.b;

    FragColor = vec4(clamp(RGB, 0.0, 1.0), 1.0);
}

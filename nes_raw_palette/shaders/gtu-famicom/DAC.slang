#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    uint FrameCount;
} global;

////////////////////////////////////////////////////////
//  GTU-famicom version 0.50
//  Author: aliaspider - aliaspider@gmail.com
//  License: GPLv3      
////////////////////////////////////////////////////////

#define TO_INT2(X) int(floor(((X) * 3.0) + 0.5))
#define TO_INT3(X) int(floor(((X) * 7.0) + 0.5))
#define TO_INT4(X) int(floor(((X) * 15.0) + 0.5))

bool InColorp (int p, int color)
{
    return ((color + p) % 12 < 6);
}

float NTSCsignal(int emphasis, int level, int color, int p)
{
    float black = .518;
    float white = 1.962;

    float attenuation = 0.746;
    const float levels[8] = float[] (   0.350 , 0.518, 0.962, 1.550,
                                        1.094f, 1.506, 1.962, 1.962);
    if (color > 13)  
        level = 1;
    
    float low  = levels[0 + level];
    float high = levels[4 + level];
    
    if (color == 0) 
        low = high;
    
    if (color > 12) 
        high = low;

    float signal = InColorp(p, color) ? high : low;

    if ((bool(emphasis & 1) && InColorp(p, 0)) ||
        (bool(emphasis & 2) && InColorp(p, 4)) ||
        (bool(emphasis & 4) && InColorp(p, 8))) 
    {
        signal = signal * attenuation;
    }


    signal = (signal - black) / (white - black);

    return signal;
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float colorPhase;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;
    vec2 pos    = (vTexCoord.xy*global.OutputSize.xy) - 0.5;
    colorPhase  = 8.0001 + pos.x + pos.y * 4.0001 + global.FrameCount * 4.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float colorPhase;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec4 c = texture(Source, vTexCoord.xy);

    int color    = TO_INT4(c.x);
    int level    = TO_INT2(c.y);
    int emphasis = TO_INT3(c.z);

    float signal = NTSCsignal(emphasis, level, color, int(colorPhase + 0.5));
    FragColor    = vec4(signal);
}

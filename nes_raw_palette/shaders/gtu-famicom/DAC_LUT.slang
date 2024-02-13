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

#define GET_LEVEL(X) ((X)*(255.0f / (128.0f*(1.962f-.518f)))-(.518f / (1.962f-.518f)))

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float colorPhase;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    vec2 pos = (vTexCoord.xy*global.OutputSize.xy) - 0.5;
    colorPhase = 8.0001 + pos.x + pos.y * 4.0001 + global.FrameCount * 4.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float colorPhase;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D nestable;

void main()
{
    vec4 c = texture(Source, vTexCoord.xy);

    vec2 pixmapCoord;
    pixmapCoord.x = c.x * (15.0 / (16.0 * 4.0)) + c.y * (3.0 / 4.0) +(0.5 / (16.0 * 4.0));
    pixmapCoord.y = 1.0 - (floor(mod(colorPhase + 0.5, 12.0)) / (12.0 * 8.0) + c.z * (7.0 / 8.0) + (0.5 / (12.0 * 8.0)));

    FragColor = vec4(GET_LEVEL(texture(nestable, pixmapCoord.xy).r));//vec4(signal);
}

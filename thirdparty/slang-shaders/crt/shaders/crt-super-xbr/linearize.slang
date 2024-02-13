#version 450

layout(push_constant) uniform Push
{
    float INPUT_GAMMA;
} params;

#pragma parameter INPUT_GAMMA "Input Gamma" 2.4 2.0 2.6 0.02

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
} global;

#pragma stage vertex
layout(location = 0) in  vec4  Position;
layout(location = 1) in  vec2  TexCoord;
layout(location = 0) out vec2  vTexCoord;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;
}

#pragma stage fragment
#pragma format R8G8B8A8_SRGB
layout(location = 0) in  vec2  vTexCoord;
layout(location = 0) out vec4  FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec3 color = texture(Source, vTexCoord).rgb;

    FragColor  = vec4(pow(color, vec3(params.INPUT_GAMMA)), 1.0);
}

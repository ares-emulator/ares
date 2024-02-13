#version 450

layout(push_constant) uniform Push
{
    float GLOW_WHITEPOINT;
    float GLOW_ROLLOFF;
} params;

#pragma parameter GLOW_WHITEPOINT "Glow Whitepoint" 1.0 0.5 1.1 0.02
#pragma parameter GLOW_ROLLOFF "Glow Rolloff" 3.0 1.2 6.0 0.1

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
    vec3 color  = 1.15 * texture(Source, vTexCoord).rgb;
    vec3 factor = clamp(color / params.GLOW_WHITEPOINT, 0.0, 1.0);

    FragColor = vec4(pow(factor, vec3(params.GLOW_ROLLOFF)), 1.0);
}

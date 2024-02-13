#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
} global;

#pragma stage vertex
layout(location = 0) in  vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
#pragma format R8G8B8A8_SRGB
layout(location = 0) in  vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define GLOW_FALLOFF 0.35
#define TAPS 4

#define kernel(x) exp(-GLOW_FALLOFF * (x) * (x))

void main()
{
    vec3 col = vec3(0.0);
    float dx = 4.0 * global.SourceSize.z; // Mipmapped

    float k_total = 0.0;
    for (int i = -TAPS; i <= TAPS; i++)
    {
        float k  = kernel(i);
        k_total += k;
        col     += k * texture(Source, vTexCoord + vec2(float(i) * dx, 0.0)).rgb;
    }

    FragColor = vec4(col / k_total, 1.0);
}

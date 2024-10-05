#version 450

layout(push_constant) uniform Push
{
    float GLOW_RADIUS;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
} global;

#pragma parameter GLOW_RADIUS "Glow Radius" 4.0 2.0 4.0 0.1

#define GLOW_RADIUS params.GLOW_RADIUS

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


vec3 get_bloom(sampler2D src, vec2 tex, vec2 dxy) {
    vec3  bloom = vec3(0.0);

    const float k[9] = {
        0.001234403, 0.014304688, 0.082317798,
        0.235235546, 0.33381513,  0.235235546,
        0.082317798, 0.014304688, 0.001234403
    };

    bloom += k[0] * texture(src, tex - 4.0 * dxy).rgb;
    bloom += k[1] * texture(src, tex - 3.0 * dxy).rgb;
    bloom += k[2] * texture(src, tex - 2.0 * dxy).rgb;
    bloom += k[3] * texture(src, tex -       dxy).rgb;
    bloom += k[4] * texture(src, tex           ).rgb;
    bloom += k[5] * texture(src, tex +       dxy).rgb;
    bloom += k[6] * texture(src, tex + 2.0 * dxy).rgb;
    bloom += k[7] * texture(src, tex + 3.0 * dxy).rgb;
    bloom += k[8] * texture(src, tex + 4.0 * dxy).rgb;

    return bloom;
}

void main()
{
    vec2 dx = vec2(GLOW_RADIUS*global.SourceSize.z, 0.0); // Mipmapped

    vec3 bloom = get_bloom(Source, vTexCoord, dx);

    FragColor = vec4(bloom, 1.0);
}

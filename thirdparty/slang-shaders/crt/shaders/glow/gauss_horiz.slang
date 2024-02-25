#version 450

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
layout(location = 1) out float data_pix_no;
layout(location = 2) out float data_one;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;

    data_pix_no = vTexCoord.x * global.SourceSize.x;
    data_one    = global.SourceSize.z;
}

#pragma stage fragment
#pragma format R8G8B8A8_SRGB
layout(location = 0) in  vec2  vTexCoord;
layout(location = 1) in  float data_pix_no;
layout(location = 2) in  float data_one;
layout(location = 0) out vec4  FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define INV_SQRT_2_PI 0.38 // Doesn't have to be accurate.

#define HORIZ_GAUSS_WIDTH 0.5

void main()
{
    float texel      = floor(data_pix_no);
    float phase      = data_pix_no - texel;
    float base_phase = phase - 0.5;
    vec2 tex         = vec2((texel + 0.5) * global.SourceSize.z, vTexCoord.y);

    vec3 col = vec3(0.0);
    for (int i = -2; i <= 2; i++)
    {
        float phase = base_phase - float(i);
        float g = INV_SQRT_2_PI * exp(-0.5 * phase * phase / (HORIZ_GAUSS_WIDTH * HORIZ_GAUSS_WIDTH)) / HORIZ_GAUSS_WIDTH;
        col += texture(Source, tex + vec2(float(i) * data_one, 0.0)).rgb * g;
    }

    FragColor = vec4(col, 1.0);
}

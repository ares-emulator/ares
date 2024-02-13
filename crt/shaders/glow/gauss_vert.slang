#version 450

layout(push_constant) uniform Push
{
    float BOOST;
} params;

#pragma parameter BOOST "Color Boost" 1.0 0.5 1.5 0.02

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
layout(location = 1) out vec2  data_pix_no;
layout(location = 2) out float data_one;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;

    data_pix_no = vTexCoord * global.SourceSize.xy - vec2(0.0, 0.5);
    data_one    = global.SourceSize.w;
}

#pragma stage fragment
#pragma format R8G8B8A8_SRGB
layout(location = 0) in  vec2  vTexCoord;
layout(location = 1) in  vec2  data_pix_no;
layout(location = 2) in  float data_one;
layout(location = 0) out vec4  FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define CRT_GEOM_BEAM 1

vec3 beam(vec3 color, float dist)
{
#if CRT_GEOM_BEAM
    vec3 wid     = 2.0 + 2.0 * pow(color, vec3(4.0));
    vec3 weights = vec3(abs(dist) * 3.333333333);

    return 2.0 * color * exp(-pow(weights * inversesqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid);
#else
    float reciprocal_width = 4.0;
    vec3 x = dist * reciprocal_width;

    return 2.0 * color * exp(-0.5 * x * x) * reciprocal_width;
#endif
}

void main()
{
    vec2  texel = floor(data_pix_no);
    float phase = data_pix_no.y - texel.y;
    vec2  tex   = vec2(texel + 0.5) * global.SourceSize.zw;

    vec3 top    = texture(Source, tex + vec2(0.0, 0 * data_one)).rgb;
    vec3 bottom = texture(Source, tex + vec2(0.0, 1 * data_one)).rgb;

    float dist0 = phase;
    float dist1 = 1.0 - phase;

    vec3 scanline = vec3(0.0);

    scanline += beam(top,    dist0);
    scanline += beam(bottom, dist1);

    FragColor = vec4(params.BOOST * scanline * 0.869565217391304, 1.0);
}


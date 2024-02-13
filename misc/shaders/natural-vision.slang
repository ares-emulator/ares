#version 450

/*
   ShadX's Natural Vision Shader

   Ported and tweaked by Hyllian - 2016
   parameterized by Sp00kyFox

*/

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

layout(push_constant) uniform push_constant
{
    vec4 SourceSize;
    vec4 OutputSize;
    float GIN;
    float GOUT;
    float Y;
    float I;
    float Q;
} params;

#pragma parameter GIN   "NaturalVision Gamma In"    2.2 0.0 10.0 0.05
#pragma parameter GOUT  "NaturalVision Gamma Out"   2.2 0.0 10.0 0.05
#pragma parameter Y "NaturalVision Luminance"   1.1 0.0 10.0 0.01
#pragma parameter I "NaturalVision Orange-Cyan" 1.1 0.0 10.0 0.01
#pragma parameter Q "NaturalVision Magenta-Green"   1.1 0.0 10.0 0.01

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

/*    VERTEX_SHADER    */
void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

const mat3x3 RGBtoYIQ = mat3x3( 0.299,     0.587,     0.114,
                                0.595716, -0.274453, -0.321263,
                                0.211456, -0.522591,  0.311135);

const mat3x3 YIQtoRGB = mat3x3( 1,  0.95629572,  0.62102442,
                                1, -0.27212210, -0.64738060,
                                1, -1.10698902,  1.70461500);

const vec3 YIQ_lo = vec3(0, -0.595716, -0.522591);
const vec3 YIQ_hi = vec3(1,  0.595716,  0.522591);


void main()
{
    vec3 c = texture(Source, vTexCoord).xyz;

    c = pow(c, vec3(params.GIN));
    c = c * RGBtoYIQ;
    c = vec3(pow(c.x, params.Y), c.y*params.I, c.z*params.Q);
    c = clamp(c, YIQ_lo, YIQ_hi);
    c = c * YIQtoRGB;
    c = pow(c, vec3(1.0/params.GOUT));

    FragColor = vec4(c, 1.0);
}

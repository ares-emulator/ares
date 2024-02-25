#version 450
#include "parameters.inc"

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   float GAMMA;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

vec3 gamma(vec3 v)
{
   return pow(v, vec3(global.GAMMA));
}

void main()
{
   FragColor = vec4(gamma(texture(Source, vTexCoord).rgb), 1.0);
}

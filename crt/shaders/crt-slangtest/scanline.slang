#version 450

#include "common.inc"
#include "parameters.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

vec3 invgamma(vec3 v)
{
   return pow(clamp(v, vec3(0.0), vec3(1.0)), vec3(1.0 / global.OUT_GAMMA));
}

float luma(vec3 col)
{
   return dot(col, vec3(0.29, 0.60, 0.11));
}

void main()
{
   vec2 tex = vTexCoord * global.SourceSize.xy;

   float fraction = fract(tex.y) - 0.5;
   tex.y = floor(tex.y) + 0.5;
   tex = tex * global.SourceSize.zw;

#define TEX(x, y) textureLodOffset(Source, tex, 0.0, ivec2(x, y)).rgb
   vec3 l0 = TEX(0, -1);
   vec3 l1 = TEX(0,  0);
   vec3 l2 = TEX(0,  1);

   vec3 dist = (3.5 - 1.0 * vec3(luma(l0), luma(l1), luma(l2))) * (fraction + vec3(+1.0, 0.0, -1.0));
   dist = exp2(-dist * dist);

   vec3 color =
      dist.x * l0 +
      dist.y * l1 +
      dist.z * l2;

   FragColor = vec4(invgamma(global.BOOST * color), 1.0);
}

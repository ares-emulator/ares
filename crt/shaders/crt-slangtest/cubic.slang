#version 450

#include "common.inc"

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

void main()
{
   float u = vTexCoord.x * global.SourceSize.x - 0.5;
   float a = fract(u);
   vec2 tex = vec2((floor(u) + 0.5) * global.SourceSize.z, vTexCoord.y);

#define TEX(x, y) textureLodOffset(Source, tex, 0.0, ivec2(x, y)).rgb

   vec3 i0 = TEX(-1, 0);
   vec3 i1 = TEX( 0, 0);
   vec3 i2 = TEX(+1, 0);
   vec3 i3 = TEX(+2, 0);

   float a2 = a * a;
   float a3 = a2 * a;

   vec3 color = i1 +
      (i2 - i0) * 0.5 * a +
      (i0 - (2.5 * i1) + (2.0 * i2) - (0.5 * i3)) * a2 +
      ((i3 - i0) + 3.0 * (i1 - i2)) * 0.5 * a3;

   FragColor = vec4(color, 1.0);
}

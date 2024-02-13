#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#include "params.inc"

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
layout(set = 0, binding = 3) uniform sampler2D Original;
layout(set = 0, binding = 4) uniform sampler2D SamplerBloom2;

#include "frag_funcs.inc"

void main()
{
   vec4 bloom = vec4(0.0);

   const vec2 offset[8] = {
      vec2(0.707, 0.707),
      vec2(0.707, -0.707),
      vec2(-0.707, 0.707),
      vec2(-0.707, -0.707),
      vec2(0.0, 1.0),
      vec2(0.0, -1.0),
      vec2(1.0, 0.0),
      vec2(-1.0, 0.0)
   };

   for (int i = 0; i < 8; i++)
   {
      vec2 bloomuv = offset[i] * PixelSize * 8.;
      bloomuv += vTexCoord.xy;
      bloom += textureLod(SamplerBloom2, vec2(bloomuv), 0.);
   }

   bloom *= 0.5; // brighten up the sample, it will lose brightness in H/V Gaussian blur
   FragColor = bloom;
}
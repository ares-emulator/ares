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

#include "frag_funcs.inc"

void main()
{
   vec4 bloom = vec4(0.);
   vec4 tempbloom = vec4(0.);
   vec2 bloomuv = vec2(0.);

   const vec2 offset[4] = {
      vec2(1.0, 1.0),
      vec2(1.0, 1.0),
      vec2(-1.0, 1.0),
      vec2(-1.0, -1.0)
   };

   for (int i = 0; i < 4; i++)
   {
      bloomuv = offset[i] * PixelSize * 2.;
      bloomuv += vTexCoord.xy;
      tempbloom = textureLod(BackBuffer, vec2(bloomuv.xy), 0.);
      tempbloom.w = max(0., dot(tempbloom.xyz, vec3(0.333)) - global.fAnamFlareThreshold);
      tempbloom.xyz = max(vec3(0.), tempbloom.xyz - vec3(global.fBloomThreshold)); 
      bloom += tempbloom;
   }

   bloom *= 0.25;
   FragColor = bloom;
}
#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This shader does a 1D gaussian blur using a 13-tap filter.

#include "slang_params.inc"
#define inTexCoord vTexCoord


// 13-tap gaussian kernel coefficients for bilinear shading, optimized to only require 7 texture samples by taking
//  advantage of linear texture filtering. These coefficients were generated using https://drilian.com/gaussian-kernel/

/*
  The actual coefficients for this blur are:

  1.107819053e-2
  2.478669128e-2
  4.790462288e-2
  7.997337680e-2
  1.153247662e-1
  1.436510723e-1
  1.545625599e-1
  1.436510723e-1
  1.153247662e-1
  7.997337680e-2
  4.790462288e-2
  2.478669128e-2
  1.107819053e-2
*/


//DECLARE_TEXTURE2D(g_sourceTex, g_sampler);
layout(set = 0, binding = 2) uniform sampler2D Source;
// access the previous pass, since we're separate horz and vert
#define g_sourceTex Source

const int k_sampleCount = 7;
const float k_coeffs[k_sampleCount] = float[k_sampleCount](
  3.586488181e-2,
  1.278779997e-1,
  2.589758386e-1,
  1.545625599e-1,
  2.589758386e-1,
  1.278779997e-1,
  3.586488181e-2
);

const float k_offsets[k_sampleCount] = float[k_sampleCount](
  -5.308886854,
  -3.374611919,
  -1.445310910,
  0.000000000,
  1.445310910,
  3.374611919,
  5.308886854
);

// Blur a texture along the blur direction (for a horizontal blur, use (1, 0) and for vertical use (0, 1)), centered at
//  "centerTexCoord" (which is in standard [0..1] texture space).
vec4 Blur(vec2 centerTexCoord, vec2 blurDirection)
{
  ivec2 dim = ivec2(params.SourceSize.xy);

  vec4 v = vec4(0., 0., 0., 0.);
  for (int i = 0; i < k_sampleCount; i++)
  {
    vec2 c = centerTexCoord + blurDirection / vec2(dim) * k_offsets[i];
    v += texture(g_sourceTex, c) * k_coeffs[i];
  }

  return v;
}

  // The direction to blur along. Should be (1, 0) to do a horizontal blur and (0, 1) to do a vertical blur.
  vec2 g_blurDir = vec2(0., 1.);


void main()
{
  FragColor = Blur(inTexCoord, g_blurDir);
}

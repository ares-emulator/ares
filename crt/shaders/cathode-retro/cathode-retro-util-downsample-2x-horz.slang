#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Downsample an input image by 2x along a given axis by using a lanczos filter.

#include "slang_params.inc"
#include "cathode-retro-util-lanczos.inc"
#define inTexCoord vTexCoord

//DECLARE_TEXTURE2D(g_sourceTexture, g_sampler);
layout(set = 0, binding = 2) uniform sampler2D Source;
#define g_sourceTexture Source

  // The direction that we're downsampling along. Should either be (1, 0) to downsample to a half-width texture or
  //  (0, 1) to downsample to a half-height texture.
  vec2 g_filterDir = vec2(1., 0.);


void main()
{
  FragColor = Lanczos2xDownsample(
    g_sourceTexture,
    inTexCoord,
    g_filterDir);
}

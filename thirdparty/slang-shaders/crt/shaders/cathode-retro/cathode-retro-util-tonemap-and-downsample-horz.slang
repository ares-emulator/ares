#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This shader does a tonemap and 1D downsample of a texture, which is intended to be for the diffusion emulation in
//  the CRT side of the whole Cathode Retro process. It's worth noting that in practice we're not doing a true 2x
//  downsample, it's going to be something in that ballpark, but not exact. However, the output of this shader is going
//  to be blurred so it doesn't actually matter in practice.

#include "slang_params.inc"
#include "cathode-retro-util-lanczos.inc"
#define inTexCoord vTexCoord

//DECLARE_TEXTURE2D(g_sourceTexture, g_sampler);
layout(set = 0, binding = 2) uniform sampler2D Source;
#define g_sourceTexture Source

  // The direction we want to apply the downsample.
  vec2 g_downsampleDir = vec2(1., 0.);
  float g_minLuminosity = 1.0 - global.minlum;

  float g_colorPower = 2.0 - global.colorpower;


void main()
{
  vec4 samp = Lanczos2xDownsample(
    g_sourceTexture,
    inTexCoord,
    g_downsampleDir);

  // Calculate the luminosity of the input.
  float inLuma = dot(samp.rgb, vec3(0.30, 0.59, 0.11));

  // Calculate the desired output luminosity.
  float outLuma = (inLuma - g_minLuminosity) / (1.0 - g_minLuminosity);
  outLuma = pow(saturate(outLuma), g_colorPower);

  // Apply the luminosity scaling.
  samp.rgb *= outLuma / inLuma;
  FragColor = samp;
}

#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This shader does a horizontal three-tap blur or sharpen to each input scanline.


#include "slang_params.inc"
#define inTexCoord vTexCoord


// The input RGB texture that will be filtered. It should be set up for linear filtering and clamp addressing.
//DECLARE_TEXTURE2D(g_sourceTexture, g_sampler);
layout(set = 0, binding = 2) uniform sampler2D Source;
#define g_sourceTexture Source

  // This is the strength of the blur - 0.0 will leave the output texture unchanged from the input, 1.0 will do a full
  //  3-texel average, and -1 will do a very extreme sharpen pass.
  float g_blurStrength = -global.blurStrength;

  // The step between each sample, in texels. Usually scaled to be relative to the original input, which may have had a
  //  different resolution than our input texture due to the NTSC generation/decode process (if there's no original
  //  input, i.e. we're not using the NTSC signal generator and have gotten a signal straight from a real NTSC signal,
  //  then you'd just want to pick some nice-on- average value instead)
  float g_stepSize = global.stepSize;


void main()
{
  vec2 inputTexelSize = vec2(ddx(inTexCoord).x, ddy(inTexCoord).y);

  // Just a simple three-tap 1D filter.
  // $TODO: This could absolutely be done with two samples instead of 3, by taking advantage of linear filtering the
  //  same way we do in the gaussian/lanczos filters.
  float blurSide = g_blurStrength / 3.0;
  float blurCenter = 1.0 - 2.0 * blurSide;
  vec4 color = texture(g_sourceTexture, (inTexCoord - vec2(g_stepSize, 0) * inputTexelSize))
                 * blurSide
               + texture(g_sourceTexture, (inTexCoord))
                 * blurCenter
               + texture(g_sourceTexture, (inTexCoord + vec2(g_stepSize, 0) * inputTexelSize))
                 * blurSide;

  FragColor = color;
}

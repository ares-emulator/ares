#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This shader applies ghosting and noise to an SVideo or Composite signal.
//
// Noise is just that, some randomized signal offset per sample to simulate analog error in the signal.
//
// Ghosting basically is what happens when a copy of your signal "skips" its intended path through the cable and mixes
//  in with your normal signal (like an EM leak of the signal) and is basically a pre-echo of the signal. So as a very
//  rough approximation of this, we'll just blend in an offset, blurred, and scaled version of the original signal.


#include "slang_params.inc"
#include "cathode-retro-util-noise.inc"
#define inputTexCoord vTexCoord

// The texture to apply artifacts to. Should either be:
//  - a 1-channel composite signal
//  - a 2-channel luma/chroma S-Video signal
//  - a 2-channel doubled composite texture (two composite versions of the same frame for temporal artifact reduction)
//  - a 4-channel luma/chroma/luma/chroma doubled S-Video signal (two S-Video versions of the same frame for temporal
//    artifact reduction)
// This sampler should be set to linear sampling, and either clamp addressing or perhaps border mode, depending.
//DECLARE_TEXTURE2D(g_sourceTexture, g_sampler);
layout(set = 0, binding = 2) uniform sampler2D Source;
#define g_sourceTexture Source

  // This represents how much ghosting is visible - 0.0 is "no ghosting" and 1.0 is "the ghost is as strong as the
  //  original signal."
  float g_ghostVisibility = global.ghost_vis;

  // This is the distance (in colorburst cycles) before the original signal that ghosting should appear.
  float g_ghostDistance = global.ghost_dist;

  // How far to blur the ghost, in colorburst cycles.
  float g_ghostSpreadScale = global.ghost_spread;

  // The strength of noise to add to the signal - 0.0 means no noise, 1.0 means, just, a lot of noise. So much.
  //  Probably too much.
  float g_noiseStrength = global.noise_strength;

  // An integer seed used to generate the per-output-texel noise. This can be a monotonically-increasing value, or
  //  random every frame, just as long as it's different every frame so that the noise changes.
  uint g_noiseSeed = uint(((params.noise_seed) * float(params.FrameCount)));

  // The width of the input signal texture
  uint g_signalTextureWidth = uint(params.SourceSize.x);

  // The number of scanlines in this field of NTSC video
  uint g_scanlineCount = (params.OriginalSize.y < 400.) ? uint(params.OriginalSize.y) : uint(params.OriginalSize.y * 0.5);

  // How many samples (texels along a scanline) there are per colorburst cycle (the color wave in the composite signal)
  uint g_samplesPerColorburstCycle = uint(global.cb_samples);

void main()
{
  vec4 signal = texture(g_sourceTexture, inputTexCoord);
  if (g_ghostVisibility != 0)
  {
    // Calculate the center sample position of our ghost based on our input params, as well as how far to spread our
    //  sampling for the blur
    vec2 ghostCenterCoord = inputTexCoord
      + vec2(g_ghostDistance * g_samplesPerColorburstCycle, 0.) / vec2(g_signalTextureWidth, g_scanlineCount);
    vec2 ghostSampleSpread = vec2(g_ghostSpreadScale * g_samplesPerColorburstCycle, 0)
      / vec2(g_signalTextureWidth, g_scanlineCount);

    // The following is a 9-tap gaussian, written as 5 samples using bilinear interpolation to sample two taps at once:
    // 0.00761441700, 0.0360749699, 0.109586075, 0.2134445460,
    // 0.26655998800, 0.2134445460, 0.109586075, 0.0360749699, 0.00761441700
    vec4 ghost;
    ghost =  texture(g_sourceTexture, ghostCenterCoord - ghostSampleSpread * 1.174285279339)
      * 0.0436893869;
    ghost += texture(g_sourceTexture, ghostCenterCoord - ghostSampleSpread * 1.339243613069)
      * 0.323030611;
    ghost += texture(g_sourceTexture, ghostCenterCoord)
      * 0.266559988;
    ghost += texture(g_sourceTexture, ghostCenterCoord + ghostSampleSpread * 1.339243613069)
      * 0.323030611;
    ghost += texture(g_sourceTexture, ghostCenterCoord + ghostSampleSpread * 1.174285279339)
      * 0.0436893869;

    signal += ghost * g_ghostVisibility;
  }

  // Also add some noise for each texel. Use the noise seed and our x/y position to c
  vec2 pixelIndex = inputTexCoord
    * vec2(g_signalTextureWidth / (g_samplesPerColorburstCycle * 2.0 / 3.0), g_scanlineCount);
  float xFrac = fract(pixelIndex.x);
  pixelIndex = floor(pixelIndex);

  // Do a little perlin-style interpolation to make the noise seem more analogue. This is one of those things that
  //  isn't strictly accurate (like I would prefer all the things be), but it *looks* right to me.
  float noiseL = Noise2D(pixelIndex, float(g_noiseSeed));
  float noiseR = Noise2D(pixelIndex + vec2(1.0, 0.0), float(g_noiseSeed));
  float noise = lerp(noiseL, noiseR, xFrac) * 2.0 - 1.0;

  // Finally we'll scale and bias the noise and return it added to our signal.
  FragColor = (signal + noise * g_noiseStrength) / (1.0 + g_ghostVisibility);
}

#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This shader generates the phase of the colorburst for each scanline.
//
// Basically, outside of the visible portion of a color NTSC scanline, there is a colorburst which the TV uses to tell
//  which color is the reference phase (which is yellow in color).  We're not generating a full NTSC signal, but we
//  still need to generate this data, and to do that we have a set of emulated timings for the virtual system that is
//  generating our output.


#include "slang_params.inc"
#include "cathode-retro-util-tracking-instability.inc"
#define inTexCoord vTexCoord

  // This is the colorburst phase (in fractional multiples of the colorburst wavelength) for the first scanline in our
  //  generated signal.
  float g_initialFrameStartPhase = global.cb_first_start;

  // This is the colorburst phase for the first scanline of the previous frame - this is used if we're generating two
  //  sets of phase information for use with temporal artifact reduction.
  float g_prevFrameStartPhase = global.cb_last_start;

  // The amount that the phase increments every scanline, in (fractional) multiples of the colorburst wavelength.
  float g_phaseIncrementPerScanline = global.cb_phase_inc;

  // How many samples (texels along a scanline) there are per colorburst cycle (the color wave in the composite signal)
  uint g_samplesPerColorburstCycle = uint(global.cb_samples);

  // The scale of any picture instability (horizontal scanline-by-scanline tracking issues). This is used to ensure the
  //  phase values generated line up with the offset of the texture sampling that happens in RGBToSVideoOrComposite.
  //  Must match the similarly-named value in cathode-retro-generator-rgb-to-svideo-or-composite.
  float g_instabilityScale = global.horz_track_scale;

  // A seed for the noise used to generate the scanline-by-scanline picture instability. Must match the simiarly-named
  //  value in cathode-retro-generator-rgb-to-svideo-or-composite.
  uint g_noiseSeed = uint(mod(params.FrameCount, 13637));

  // The width of the signal texture that is being generated, in texels.
  uint g_signalTextureWidth = uint(params.OriginalSize.x);

  // The number of scanlines for this field of video.
  uint g_scanlineCount = (params.OriginalSize.y < 400.) ? uint(params.OriginalSize.y) : uint(params.OriginalSize.y * 0.5);


void main()
{
  uint scanlineIndex = uint(round(inTexCoord.y * g_scanlineCount - 0.5));

  // Calculate the phase for the start of the given scanline.
  vec2 phases = vec2(g_initialFrameStartPhase, g_prevFrameStartPhase)
    + g_phaseIncrementPerScanline * float(scanlineIndex);

  // Offset by half a cycle so that it lines up with the *center* of the filter instead of the left edge of it.
  phases += 0.5 / float(g_samplesPerColorburstCycle);

  // Finally, offset by the scaled instability so that if there are color artifacts, they'll have the correct phase.
  // $TODO: This is actually slightly wrong, and I'm assuming it's due to the way the texture sampling works when
  //  generating the signal texture.  This value is slightly too high when we're at half a colorburst phase offset (by
  //  something like 0.05, found by manually tweaking the tint slider to get a close match), and I'm not entirely sure
  //  why. Intuition says that because we're not oversampling the RGB texture to generate the signal texture, any
  //  intentional linear-blending texture offsets (like for 1bpp CGA modes) aren't actually offset *exactly* halfway at
  //  that point, and so the calculated phase on NTSC demodulation is slightly off from true.
  float instability = CalculateTrackingInstabilityOffset(
    scanlineIndex,
    g_noiseSeed,
    g_instabilityScale,
    g_signalTextureWidth);
  phases += instability * float(g_signalTextureWidth) / float(g_samplesPerColorburstCycle);

  FragColor = vec4(fract(phases), 0, 0);
}

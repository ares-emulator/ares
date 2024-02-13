#include "cathode-retro-util-noise.inc"

// Calculate the instability noise per scanline
float CalculateTrackingInstabilityOffset(uint scanlineIndex, uint noiseSeed, float scale, uint signalTextureWidth)
{
  return (Noise1D(float(scanlineIndex), float(noiseSeed)) - 0.5) * scale / signalTextureWidth;
}

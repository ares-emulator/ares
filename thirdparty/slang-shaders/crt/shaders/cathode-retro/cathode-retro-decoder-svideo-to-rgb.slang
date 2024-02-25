#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This signal takes an input S-Video signal and converts it into a set of YIQ color space values.
//
// This process is called NTSC color demodulation, which is a type of QAM demodulation.
//
// An NTSC signal (either composite or S-Video) is really just encoded YIQ data, where the Y value is encoded as luma
//  and IQ is encoded as a waveform in the chroma. Since this shader takes an effective S-Video signal as input, the Y
//  component is easy - it's just the luma channel of the input.
//
// Just like filtering the luma from the chroma (see the comment in CompositeToSVideo.hlsl), rather than doing any sort
//  of fancy filtering here, the absolute best results I've gotten were from a super-basic average (which very nicely
//  removes a very specific frequency and all integer multiples of it), which is also exactly what we need here.
//
// Additionally, if we want temporal artifact reduction, we're doing two of this calculation (we have a
//  (lumaA, chromaA, lumaB, chromaB) texture representing the same frame with two different color phases), and we use
//  the temporal artifact reduction value to blend between them, which reduces or eliminates alternating-phase-related
//  flickering that systems like the NES could generate.
//
// The output of this shader is a texture that contains the decoded Y, I, and Q channels in R, G, and B
//  (plus 1.0 in alpha).


#include "slang_params.inc"
#include "cathode-retro-util-box-filter.inc"


// This is a 2- or 4-component texture that contains either a single luma, chroma sample pair or two luma, chroma pairs
//  of S-Video-like signal. It's 2 components if we have no temporal artifact reduction (we're not blending two
//  versions of the same frame), 4 if we do. This sampler should be set up for linear filtering and clamped addressing.
//DECLARE_TEXTURE2D(g_sourceTexture, g_sampler);
layout(set = 0, binding = 2) uniform sampler2D IQ_separate;
#define g_sourceTexture IQ_separate

// This is a 2- or 4-component texture that contains the chroma portions of the signal modulated with the carrier
//  quadrature (basically, it's float4(chromaA * sin, chromaA * cos, chromaB * sin, chromaB * cos).
// This has been modulated in a separate pass for efficiency - otherwise we'd need to do a bunch of  sines and cosines
//  per pixel here.
//DECLARE_TEXTURE2D(g_modulatedChromaTexture, g_modulatedChromaSampler);
layout(set = 0, binding = 3) uniform sampler2D g_modulatedChromaTexture;


  // How many samples (horizontal texels) there are per each color wave cycle.
  uint g_samplesPerColorburstCycle = uint(global.cb_samples);

  // This is a value representing how saturated we want the output to be. 0 basically means we'll decode as a grayscale
  //  image, 1 means fully saturated color (i.e. the intended input saturation), and you could even set values greater
  //  than 1 to oversaturate.
  // This corresponds to the saturation dial of a CRT TV.
  // $NOTE: This value should be pre-scaled by the g_brightness value, so that if brightness is 0, saturation is always
  //  0 - otherwise, you get weird output values where there should have been nothing visible but instead you get a
  //  pure color instead.
  float g_saturation = global.cat_sat * saturate(global.cat_bright);

  // This is a value representing the brightness of the output. a value of 0 means we'll output pure black, and 1 means
  //  "the intended brightness based on the input signal". Values above 1 will over-brighten the output.
  //  This corresponds to the brightness dial of a CRT TV.
  float g_brightness = global.cat_bright;

  // This is the luma value of the input signal that represents black. For our synthetic signals it's typically 0.0,
  //  but from a real NTSC signal this can be some other voltage level, since a voltage of 0 typically indicates a
  //  horizontal or vertical blank instead. This is calculated from/generated with the composite or S-Video signal we
  //  were given.
  float g_blackLevel = global.cat_black_lvl;

  // This is the luma value of the input signal that represents brightest white.  For our synthetic signals it's
  //  typically 1.0, but from a real NTSC signal (or if we've applied some signal artifacts like ghosting) it could be
  //  some other value. This is calculated from/generated with the composite or S-Video signal we were given.
  float g_whiteLevel = global.cat_white_lvl;

  // A [0..1] value indicating how much we want to blend in an alternate version of the generated signal to adjust for
  //  any artifacting between successive frames. 0 means we only have (or want to use) a single input luma/chroma pair.
  //  A value > 0 means we are going to blend the results of two parallel-computed versions of our YIQ values, with a
  //  value of 1.0 being a pure average of the two.
  float g_temporalArtifactReduction = global.temp_artifact_blend;

  // The width of the input signal (including any side padding)
  uint g_inputWidth = uint(params.SourceSize.x);

  // The width of the output RGB image (should be the width of the input signal minus the side padding)
  uint g_outputWidth = uint(params.OutputSize.x) - uint(global.sig_pad);


const float k_pi = 3.141592653;


void main()
{
  vec2 inTexCoord = vTexCoord.xy;
  inTexCoord.x = (inTexCoord.x - 0.5) * float(g_outputWidth) / float(g_inputWidth) + 0.5;

  vec2 inputTexelSize = vec2(ddx(inTexCoord).x, ddy(inTexCoord).y);

  // Get the index of our x sample.
  uint sampleXIndex = uint(floor(inTexCoord.x / inputTexelSize.x));

  // This is the chroma decode process, it's a QAM demodulation.
  //  You multiply the chroma signal by a reference waveform and its quadrature (Basically, sin and cos at a given
  //  time) and then filter out the chroma frequency (here done by a box filter (an average)). What you're left with
  //  are the approximate I and Q color space values for this part of the image.
  vec2 Y = texture(g_sourceTexture, inTexCoord).xz;


  // We're going to sample chroma at double our actual colorburst cycle to eliminate some deeply rough artifacting on
  //  the edges. In this case we're going to average by DOUBLE the color burst cycle - doing just a single cycle ends
  //  up with a LOT of edge artifacting on things, which are too strong. 2x is much softer but not so large that you
  //  can really notice that it's adding a ton of extra blurring on the color channel (it needs to be an integer
  //  multiple of our cycle sample count for the filtering to work).
  // $TODO: This could actually be reduced to 1 when we're doing an decode of a signal that was not originally
  //  composite, since there are no areas where luma changes will have crept into the color channel, which is typically
  //  the artifacting we see.
  uint filterWidth = g_samplesPerColorburstCycle * 2U;

  vec4 unused; // BoxFilter outputs a center sample, but we don't care about that.
  vec4 IQ = BoxFilter(
    g_modulatedChromaTexture, 
    vec2(1.0 / float(g_inputWidth), 0.0),
    2U * g_samplesPerColorburstCycle,
    inTexCoord,
    unused);

  // Adjust our components, first Y to account for the signal's black/white level (and user-chosen brightness), then IQ
  //  for saturation (Which should also include the signal's brightness scale)
  Y = (Y - g_blackLevel) / (g_whiteLevel - g_blackLevel) * g_brightness;
  IQ *= vec4(g_saturation, g_saturation, g_saturation, g_saturation);

  // we have 1 or 2 components of Y, and 2 or 4 of IQ. Blend them together based on our temporal aliasing reduction to
  //  get our final decoded YIQ values.
  Y.x = lerp(Y.x, Y.y, g_temporalArtifactReduction * 0.5);
  IQ.xy = lerp(IQ.xy, IQ.zw, g_temporalArtifactReduction * 0.5);

  // Do some gamma adjustments (values effectively based on eyeballing the results of NTSC signals from NES, SNES, and
  //  Genesis consoles)
  Y.x = pow(saturate(Y.x), 2.0 / 2.2);
  float iqSat = saturate(length(IQ.xy));
  IQ.xy *= pow(iqSat, 2.0 / 2.2) / max(0.00001, iqSat);

  // Finally, run the YIQ values through the standard (SMPTE C) YIQ to RGB conversion matrix
  //  (from https://en.wikipedia.org/wiki/YIQ)
  vec3 yiq = vec3(Y.x, IQ.xy);
  FragColor = vec4(
    dot(yiq, vec3(1.0, 0.946882, 0.623557)),
    dot(yiq, vec3(1.0, -0.274788, -0.635691)),
    dot(yiq, vec3(1.0, -1.108545, 1.7090047)),
    1.0);
}

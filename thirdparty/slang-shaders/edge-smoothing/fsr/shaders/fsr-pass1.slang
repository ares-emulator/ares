#version 450
// SPDX-License-Identifier: Unlicense
#pragma name FSR_RCAS

// FSR - [RCAS] ROBUST CONTRAST ADAPTIVE SHARPENING

// filter_linear1 = true
// scale_type1 = source
// scale1 = 1.0

#pragma parameter FSR_SHARPENING "FSR RCAS Sharpening Amount (Lower = Sharper)" 0.2 0.0 2.0 0.1
#pragma parameter FSR_FILMGRAIN "FSR LFGA Film Grain Intensity" 0.3 0.0 2.0 0.02
#pragma parameter FSR_GRAINCOLOR "FSR LFGA Film Grain Color: Gray | RGB" 1.0 0.0 1.0 1.0
#pragma parameter FSR_GRAINPDF "FSR LFGA Grain PDF Curve (0.5 = Triangular, Lower = Gaussian)" 0.3 0.1 0.5 0.05

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float FSR_SHARPENING;
    float FSR_FILMGRAIN;
    float FSR_GRAINCOLOR;
    float FSR_GRAINPDF;
} params;

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define A_GPU 1
#define A_GLSL 1
#include "ffx_a.h"

#define FSR_RCAS_F 1
AU4 con0;

AF4 FsrRcasLoadF(ASU2 p) { return AF4(texelFetch(Source, p, 0)); }
void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}

#include "ffx_fsr1.h"

// prng: A simple but effective pseudo-random number generator [0;1[
float prng(vec2 uv, float time) {
    return fract(sin(dot(uv + fract(time), vec2(12.9898, 78.233))) * 43758.5453);
}

// pdf: [-0.5;0.5[
// Removes noise modulation effect by reshaping the uniform/rectangular noise
// distribution (RPDF) into a Triangular (TPDF) or Gaussian Probability Density
// Function (GPDF).
// shape = 1.0: Rectangular
// shape = 0.5: Triangular
// shape < 0.5: Gaussian (0.2~0.4)
float pdf(float noise, float shape) {
    float orig = noise * 2.0 - 1.0;
    noise = pow(abs(orig), shape);
    noise *= sign(orig);
    noise -= sign(orig);
    return noise * 0.5;
}

void main() {
    FsrRcasCon(con0, params.FSR_SHARPENING);

    AU2 gxy = AU2(vTexCoord.xy * params.OutputSize.xy); // Integer pixel position in output.
    AF3 Gamma2Color = AF3(0, 0, 0);
    FsrRcasF(Gamma2Color.r, Gamma2Color.g, Gamma2Color.b, gxy, con0);

    // FSR - [LFGA] LINEAR FILM GRAIN APPLICATOR
    if (params.FSR_FILMGRAIN > 0.0) {
        if (params.FSR_GRAINCOLOR == 0.0) {
            float noise = pdf(prng(vTexCoord, params.FrameCount * 0.11), params.FSR_GRAINPDF);
            FsrLfgaF(Gamma2Color, vec3(noise), params.FSR_FILMGRAIN);
        } else {
            vec3 rgbNoise = vec3(
                pdf(prng(vTexCoord, params.FrameCount * 0.11), params.FSR_GRAINPDF),
                pdf(prng(vTexCoord, params.FrameCount * 0.13), params.FSR_GRAINPDF),
                pdf(prng(vTexCoord, params.FrameCount * 0.17), params.FSR_GRAINPDF)
            );
            FsrLfgaF(Gamma2Color, rgbNoise, params.FSR_FILMGRAIN);
        }
    }

    FragColor = vec4(Gamma2Color, 1.0);
}

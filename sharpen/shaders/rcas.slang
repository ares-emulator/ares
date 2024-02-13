#version 450

/*
    Robust Contrast Adaptive (RCA) Sharpening v1.0, re-implemented by fishku

    Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

    Changelog:
    v1.0: Initial release.
*/

/*
The implementation and documentation largely follow these resources:
- https://github.com/GPUOpen-Effects/FidelityFX-FSR/blob/master/ffx-fsr/ffx_fsr1.h#L602
- https://www.shadertoy.com/view/7tfSWH

RCAS is based on the following logic.
RCAS uses a 5 tap filter in a cross pattern (same as CAS),
   w                n
 w 1 w  for taps  w m e
   w                s
Where 'w' is the negative lobe weight.
 output = (w*(n+e+w+s)+m)/(4*w+1)
RCAS solves for 'w' by seeing where the signal might clip out of the {0 to 1} input range,
 0 == (w*(n+e+w+s)+m)/(4*w+1) -> w = -m/(n+e+w+s)
 1 == (w*(n+e+w+s)+m)/(4*w+1) -> w = (1-m)/(n+e+w+s-4*1)
Then chooses the 'w' which results in no clipping, limits 'w', and multiplies by the 'sharp' amount.
This solution above has issues with MSAA input as the steps along the gradient cause edge detection
issues.
So RCAS uses 4x the maximum and 4x the minimum (depending on equation) in place of the individual
taps.
As well as switching from 'm' to either the minimum or maximum (depending on side), to help in
energy conservation.
This stabilizes RCAS. RCAS does a simple highpass which is normalized against the local contrast
then shaped,
     0.25
 0.25  -1  0.25
     0.25
This is used as a noise detection filter, to reduce the effect of RCAS on grain, and focus on real
edges.
*/

#pragma parameter RCAS_SETTINGS "=== RCA v1.0 Sharpening Settings ===" 0.0 0.0 1.0 1.0
#pragma parameter RCAS_STRENGTH "Strength of RCA sharpening" 0.5 0.0 1.1 0.05
#pragma parameter RCAS_DENOISE "Suppress luma oversharpening" 1.0 0.0 1.0 1.0

layout(push_constant) uniform Push {
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    uint FrameCount;
    float RCAS_STRENGTH;
    float RCAS_DENOISE;
}
param;

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 MVP;
}
global;

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
layout(set = 0, binding = 1) uniform sampler2D Source;

// This is set at the limit of providing unnatural results for sharpening.
#define RCAS_LIMIT (0.25f - (1.0 / 16.0))

void main() {
    const vec4 offset = vec4(1, 0, 1, -1) * param.SourceSize.zzww;
    const vec3 sample_n = texture(Source, vTexCoord + offset.yz).rgb;
    const vec3 sample_w = texture(Source, vTexCoord - offset.xy).rgb;
    const vec3 sample_m = texture(Source, vTexCoord).rgb;
    const vec3 sample_e = texture(Source, vTexCoord + offset.xy).rgb;
    const vec3 sample_s = texture(Source, vTexCoord + offset.yw).rgb;

    const vec3 max_edges = max(max(sample_n, sample_s), max(sample_e, sample_w));
    const vec3 min_edges = min(min(sample_n, sample_s), min(sample_e, sample_w));
    const vec3 sum_edges = sample_n + sample_e + sample_s + sample_w;
    const vec3 edge = -0.25f * min(min_edges / max_edges, (1.0 - max_edges) / (1.0 - min_edges));
    const float edges = clamp(max(edge.r, max(edge.g, edge.b)), -RCAS_LIMIT, 0.0);
    float w = edges * param.RCAS_STRENGTH;

    if (param.RCAS_DENOISE > 0.5) {
        // Luma times 2.
        const float luma_n = 0.5 * (sample_n.b + sample_n.r) + sample_n.g;
        const float luma_w = 0.5 * (sample_w.b + sample_w.r) + sample_w.g;
        const float luma_m = 0.5 * (sample_m.b + sample_m.r) + sample_m.g;
        const float luma_e = 0.5 * (sample_e.b + sample_e.r) + sample_e.g;
        const float luma_s = 0.5 * (sample_s.b + sample_s.r) + sample_s.g;
        // Noise detection.
        float nz = 0.25 * (luma_n + luma_w + luma_e + luma_s) - luma_m;
        nz = clamp(abs(nz) / (max(max(max(max(luma_n, luma_w), luma_m), luma_e), luma_s) -
                              min(min(min(min(luma_n, luma_w), luma_m), luma_e), luma_s)),
                   0.0, 1.0);
        nz = 1.0 - 0.5 * nz;
        // Apply noise removal.
        w *= nz;
    }

    vec3 col = (sample_m + sum_edges * w) / (w * 4.0 + 1.0);
    col = clamp(col, vec3(0.0), vec3(1.0));

    FragColor = vec4(col, 1.0);
}

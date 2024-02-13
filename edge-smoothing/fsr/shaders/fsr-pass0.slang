#version 450
// SPDX-License-Identifier: Unlicense
#pragma name FSR_EASU

// FSR - [EASU] EDGE ADAPTIVE SPATIAL UPSAMPLING

// filter_linear0 = true
// scale_type0 = viewport
// scale0 = 1.0

layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
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

#define FSR_EASU_F 1
AU4 con0, con1, con2, con3;

AF4 FsrEasuRF(AF2 p) { return textureGather(Source, p, 0); }
AF4 FsrEasuGF(AF2 p) { return textureGather(Source, p, 1); }
AF4 FsrEasuBF(AF2 p) { return textureGather(Source, p, 2); }

#include "ffx_fsr1.h"

void main() {
    FsrEasuCon(con0, con1, con2, con3,
        params.SourceSize.x, params.SourceSize.y,  // Viewport size (top left aligned) in the input image which is to be scaled.
        params.SourceSize.x, params.SourceSize.y,  // The size of the input image.
        params.OutputSize.x, params.OutputSize.y); // The output resolution.

    AU2 gxy = AU2(vTexCoord.xy * params.OutputSize.xy); // Integer pixel position in output.
    AF3 Gamma2Color = AF3(0, 0, 0);
    FsrEasuF(Gamma2Color, gxy, con0, con1, con2, con3);

    FragColor = vec4(Gamma2Color, 1.0);
}

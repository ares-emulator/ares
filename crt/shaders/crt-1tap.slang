#version 450

/*
    crt-1tap v1.2 by fishku
    Copyright (C) 2023
    Public domain license (CC0)

    Extremely fast and lightweight dynamic scanline shader.
    Contrasty and sharp output. Easy to configure.
    Can be combined with other shaders.

    How it works: Uses a single texture "tap" per pixel, hence the name.
    Exploits bilinear interpolation plus local coordinate distortion to shape
    the scanline. A pseudo-sigmoid function with a configurable slope at the
    inflection point is used to control horizontal smoothness.
    Uses a clamped linear function to anti-alias the edge of the scanline while
    avoiding further branching. The final thickness is shaped with a gamma-like
    operation to control overall image contrast.
    The scanline subpixel position can be controlled to achieve the best
    sharpness and uniformity of the output given different input sizes, e.g.,
    for even and odd integer scaling.

    Changelog:
    v1.2: Better scanline sharpness; Minor cleanups.
    v1.1: Update license; Better defaults; Don't compute alpha.
    v1.0: Initial release.
*/

// clang-format off
#pragma parameter CRT1TAP_SETTINGS "=== CRT-1tap v1.2 settings ===" 0.0 0.0 1.0 1.0
#pragma parameter MIN_THICK "Scanline thickness of dark pixels" 0.3 0.0 1.4 0.05
#pragma parameter MAX_THICK "Scanline thickness of bright pixels" 0.9 0.0 1.4 0.05
#pragma parameter V_SHARP "Vertical sharpness of the scanline" 0.5 0.0 1.0 0.05
#pragma parameter H_SHARP "Horizontal sharpness of pixel transitions" 0.15 0.0 1.0 0.05
#pragma parameter SUBPX_POS "Scanline subpixel position" 0.3 -0.5 0.5 0.01
#pragma parameter THICK_FALLOFF "Reduction / increase of thinner scanlines" 0.65 0.2 2.0 0.05
// clang-format on

layout(push_constant) uniform Push {
    vec4 OutputSize;
    vec4 SourceSize;
    float MIN_THICK;
    float MAX_THICK;
    float V_SHARP;
    float H_SHARP;
    float SUBPX_POS;
    float THICK_FALLOFF;
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
layout(set = 0, binding = 2) uniform sampler2D Original;

void main() {
    float src_x_int;
    const float src_x_fract = modf(vTexCoord.x * param.SourceSize.x - 0.5, src_x_int);

    float src_y_int;
    const float src_y_fract =
        modf(vTexCoord.y * param.SourceSize.y - param.SUBPX_POS, src_y_int) - 0.5;

    // Function similar to smoothstep and sigmoid.
    const float s = sign(src_x_fract - 0.5);
    const float o = (1.0 + s) * 0.5;
    const float src_x =
        src_x_int + o - 0.5 * s * pow(2.0 * (o - s * src_x_fract), mix(1.0, 6.0, param.H_SHARP));

    const vec3 signal = texture(Source, vec2((src_x + 0.5) * param.SourceSize.z,
                                             (src_y_int + 0.5) * param.SourceSize.w))
                            .rgb;

    // Vectorize operations for speed.
    const float eff_v_sharp = 3.0 + 50.0 * param.V_SHARP * param.V_SHARP;
    const vec3 radius =
        pow(mix(param.MIN_THICK.xxx, param.MAX_THICK.xxx, signal), param.THICK_FALLOFF.xxx) * 0.5;
    FragColor.rgb =
        signal * clamp(0.25 - eff_v_sharp * (src_y_fract * src_y_fract - radius * radius),
        0.0, 1.0);
}

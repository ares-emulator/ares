#version 450

/*
    Ray traced curvature v1.0 by fishku
    Copyright (C) 2024
    Public domain license (CC0)

    This example demoes the following:
    - How to generate rays for tracing against the screen surface in the vertex
      shader.
    - How to trace the rays in the fragment shader.
    - How to achieve high-quality sampling with the found UV.

    Changelog:
    v1.0: Initial release.
*/

// If uncommented, pixel art is sampled sharply, better for stand-alone usage.
// If commented out, no coordinate distortion is done, which is better for
// appending. Enabling this option may give stronger aliasing artifacts when
// appending to other presets.
#define SHARP_PIXEL_SAMPLING

// If uncommented, mip-map levels are blended, if available, giving better AA.
#define TRILINEAR_SAMPLING

#include "../../../misc/shaders/input_transform/rotation.inc"
#include "parameters.inc"
#include "shared.inc"

layout(push_constant) uniform Push {
    vec4 OriginalSize;
    vec4 SourceSize;
    vec4 OutputSize;
    uint Rotation;
    float RT_CURV_ASPECT_H;
    float RT_CURV_ASPECT_V;
    float RT_CURV_SHAPE;
    float RT_CURV_CURVATURE;
    float RT_CURV_ZOOM;
    float RT_CURV_TILT_ANGLE_H;
    float RT_CURV_TILT_ANGLE_V;
    float RT_CURV_F;
    float RT_CURV_ROUNDED_CORNER;
    float RT_CURV_LOD_BIAS;
    float RT_CURV_APPEND;
}
param;

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; }
global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec3 prim_ray_o;
layout(location = 1) out vec3 prim_ray_d;
layout(location = 2) out vec3 plane_n;
layout(location = 3) out vec3 plane_u;
layout(location = 4) out vec3 plane_v;
layout(location = 5) out vec2 input_aspect;

void main() {
    gl_Position = global.MVP * Position;

    // Define curved object: sphere or cylinder.
    // Simplification: Assume origin of curved object (sphere / cylinder) is at
    // the origin.
    const float r =
        param.RT_CURV_CURVATURE > 1.0e-3 ? 1.0 / param.RT_CURV_CURVATURE : 1.0;
    // TODO: Why does this need an extra flip compared to the usage in
    // input_transform.inc?
    const vec2 tilt = get_rotated_vector(vec2(param.RT_CURV_TILT_ANGLE_H,
                                              param.RT_CURV_TILT_ANGLE_V),
                                         param.Rotation) *
                      (1 - 2 * (int(param.Rotation) % 2));
    const vec2 sin_tilt = sin(tilt);
    const vec2 cos_tilt = cos(tilt);
    // Simplification: Assume cylinder axis == plane_v.

    // Define input texture plane.
    // Basic vectors are:
    // n = (0, 0, -r)
    // u = (1, 0, 0)
    // v = (0, 1, 0)
    // n serves a double duty as the origin of the plane.
    // These are then rotated around x by tilt.x and then around y by tilt.y.
    plane_n = vec3(r * sin_tilt.x * cos_tilt.y, -r * sin_tilt.y,
                   -r * cos_tilt.x * cos_tilt.y);
    plane_u = vec3(cos_tilt.x, 0.0, sin_tilt.x);
    plane_v =
        vec3(sin_tilt.x * sin_tilt.y, cos_tilt.y, -cos_tilt.x * sin_tilt.y);

    // Compute aspects ratios.
    // Output aspect ratio does not need to be rotated since RA takes care of
    // output rotation.
    input_aspect =
        param.RT_CURV_ASPECT_H > 0.0 && param.RT_CURV_ASPECT_V > 0.0
            ? get_rotated_size(
                  vec2(param.RT_CURV_ASPECT_H, param.RT_CURV_ASPECT_V) /
                      max(param.RT_CURV_ASPECT_H, param.RT_CURV_ASPECT_V),
                  param.Rotation)
            : param.OriginalSize.xy /
                  max(param.OriginalSize.x, param.OriginalSize.y);
    const vec2 output_aspect =
        param.OutputSize.xy / max(param.OutputSize.x, param.OutputSize.y);

    // Simplification: Assume plane_o = plane_n
    generate_ray(TexCoord, input_aspect, output_aspect, plane_n, plane_u,
                 plane_v, param.RT_CURV_CURVATURE, param.RT_CURV_F,
                 param.RT_CURV_SHAPE, param.RT_CURV_ZOOM, prim_ray_o,
                 prim_ray_d);
}

#pragma stage fragment
layout(location = 0) in vec3 prim_ray_o;
layout(location = 1) in vec3 prim_ray_d;
layout(location = 2) in vec3 plane_n;
layout(location = 3) in vec3 plane_u;
layout(location = 4) in vec3 plane_v;
layout(location = 5) in vec2 input_aspect;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main() {
    vec2 uv = trace_ray(input_aspect, prim_ray_o, prim_ray_d, plane_n, plane_u,
                        plane_v, param.RT_CURV_CURVATURE, param.RT_CURV_SHAPE);

    // Compute rounded corner darkening.
    const vec2 q =
        input_aspect * (abs(uv - 0.5) - 0.5) + param.RT_CURV_ROUNDED_CORNER;
    const float rounded_rect_dist = min(max(q.x, q.y), 0.0) +
                                    length(max(q, 0.0)) -
                                    param.RT_CURV_ROUNDED_CORNER;
    const float corner_darkening = smoothstep(
        -max(param.OutputSize.z, param.OutputSize.w), 0.0, -rounded_rect_dist);

    // Sampling section follows.
#if defined(SHARP_PIXEL_SAMPLING) || defined(TRILINEAR_SAMPLING)
    const vec2 d_uv_dx = dFdx(uv) * param.SourceSize.xy;
    const vec2 d_uv_dy = dFdy(uv) * param.SourceSize.xy;
#endif

#ifdef SHARP_PIXEL_SAMPLING
    if (param.RT_CURV_APPEND < 0.5) {
        // Do a sharp "pixel art" sampling, following:
        // https://www.youtube.com/watch?v=d6tp43wZqps
        // Only apply pixel art sampling when not appending to avoid aliasing.
        const vec2 box_size = clamp(abs(d_uv_dx) + abs(d_uv_dy), 1.0e-6, 1.0);
        const vec2 tx = uv * param.SourceSize.xy - 0.5 * box_size;
        const vec2 tx_offset = smoothstep(1 - box_size, vec2(1.0), fract(tx));
        uv = (floor(tx) + 0.5 + tx_offset) * param.SourceSize.zw;
    }
#endif

#ifdef TRILINEAR_SAMPLING
    // Anisotropic trilinear filtering.
    // Implement in software because current implementation is broken. See:
    // https://github.com/libretro/RetroArch/issues/16567
    const float lambda_base =
        max(0.0,
            0.5 * log2(max(dot(d_uv_dx, d_uv_dx), dot(d_uv_dy, d_uv_dy)))) +
        param.RT_CURV_LOD_BIAS;
    float lambda_i;
    const float lambda_f = modf(lambda_base, lambda_i);
    FragColor = vec4(mix(textureLod(Source, uv, lambda_i).rgb,
                         textureLod(Source, uv, lambda_i + 1.0).rgb, lambda_f),
                     1.0);
#else
    FragColor = vec4(textureLod(Source, uv, param.RT_CURV_LOD_BIAS).rgb, 1.0);
#endif

    // Apply rounded corner darkening.
    FragColor.rgb *= corner_darkening;
}

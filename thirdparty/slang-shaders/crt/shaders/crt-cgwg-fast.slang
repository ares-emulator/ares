#version 450

/*
    cgwg's CRT shader

    Copyright (C) 2010-2011 cgwg, Themaister

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    (cgwg gave their consent to have their code distributed under the GPL in
    this message:

        http://board.byuu.org/viewtopic.php?p=26075#p26075

        "Feel free to distribute my shaders under the GPL. After all, the
        barrel distortion code was taken from the Curvature shader, which is
        under the GPL."
    )
*/

layout(push_constant) uniform Push
{
    float CRTCGWG_GAMMA;
} param;

#pragma parameter CRTCGWG_GAMMA "CRTcgwg Gamma" 2.7 0.0 10.0 0.01

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    uint FrameCount;
} global;

#pragma stage vertex
layout(location = 0) in  vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

/* TODO/FIXME - Wrap all these in a struct-like type so we can address
them in a struct-like way i.e. coords.c01 */
layout(location = 1) out vec2 c01; 
layout(location = 2) out vec2 c11; 
layout(location = 3) out vec2 c21;
layout(location = 4) out vec2 c31;
layout(location = 5) out vec2 c02;
layout(location = 6) out vec2 c12; 
layout(location = 7) out vec2 c22;
layout(location = 8) out vec2 c32; 
layout(location = 9) out float mod_factor;
layout(location = 10) out vec2 ratio_scale;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;

    vec2 delta = global.SourceSize.zw;
    float dx   = delta.x;
    float dy   = delta.y;

    c01 = vTexCoord + vec2(-dx, 0.0);
    c11 = vTexCoord + vec2(0.0, 0.0);
    c21 = vTexCoord + vec2(dx, 0.0);
    c31 = vTexCoord + vec2(2.0 * dx, 0.0);
    c02 = vTexCoord + vec2(-dx, dy);
    c12 = vTexCoord + vec2(0.0, dy);
    c22 = vTexCoord + vec2(dx, dy);
    c32 = vTexCoord + vec2(2.0 * dx, dy);
    mod_factor  = vTexCoord.x * global.OutputSize.x;
    ratio_scale = vTexCoord * global.SourceSize.xy;
}

#pragma stage fragment
layout(location = 0)    in vec2 vTexCoord;
layout(location = 1)    in vec2 c01; 
layout(location = 2)    in vec2 c11; 
layout(location = 3)    in vec2 c21;
layout(location = 4)    in vec2 c31;
layout(location = 5)    in vec2 c02;
layout(location = 6)    in vec2 c12; 
layout(location = 7)    in vec2 c22;
layout(location = 8)    in vec2 c32; 
layout(location = 9)    in float mod_factor;
layout(location = 10)   in vec2 ratio_scale;
layout(location = 0)    out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define TEX2D(c) texture(Source ,(c))
#define PI 3.141592653589

void main()
{
    vec2 uv_ratio = fract(ratio_scale);
    vec3 col, col2;

    mat4x3 texes0 = mat4x3(TEX2D(c01).xyz, TEX2D(c11).xyz, TEX2D(c21).xyz, TEX2D(c31).xyz);
    mat4x3 texes1 = mat4x3(TEX2D(c02).xyz, TEX2D(c12).xyz, TEX2D(c22).xyz, TEX2D(c32).xyz);

    vec4 coeffs = vec4(1.0 + uv_ratio.x, uv_ratio.x, 1.0 - uv_ratio.x, 2.0 - uv_ratio.x) + 0.005;
    coeffs      = sin(PI * coeffs) * sin(0.5 * PI * coeffs) / (coeffs * coeffs);
    coeffs      = coeffs / dot(coeffs, vec4(1.0, 1.0, 1.0, 1.0));

    vec3 weights  = vec3( 3.33 * uv_ratio.y,        uv_ratio.y *  3.33,        uv_ratio.y *  3.33);
    vec3 weights2 = vec3(-3.33 * uv_ratio.y + 3.33, uv_ratio.y * -3.33 + 3.33, uv_ratio.y * -3.33 + 3.33);

    col  = clamp(texes0 * coeffs, 0.0, 1.0);
    col2 = clamp(texes1 * coeffs, 0.0, 1.0);

    vec3 wid  = 2.0 * pow(col,  vec3(4.0, 4.0, 4.0)) + 2.0;
    vec3 wid2 = 2.0 * pow(col2, vec3(4.0, 4.0, 4.0)) + 2.0;

    col  = pow(col,  vec3(param.CRTCGWG_GAMMA));
    col2 = pow(col2, vec3(param.CRTCGWG_GAMMA));

    vec3 sqrt1 = inversesqrt(0.5 * wid);
    vec3 sqrt2 = inversesqrt(0.5 * wid2);

    vec3 pow_mul1 = weights * sqrt1;
    vec3 pow_mul2 = weights2 * sqrt2;

    vec3 div1 = 0.1320 * wid  + 0.392;
    vec3 div2 = 0.1320 * wid2 + 0.392;

    vec3 pow1 = -pow(pow_mul1, wid);
    vec3 pow2 = -pow(pow_mul2, wid2);

    weights  = exp(pow1) / div1;
    weights2 = exp(pow2) / div2;

    vec3 multi = col * weights + col2 * weights2;
    vec3 mcol  = mix(vec3(1.0, 0.7, 1.0), vec3(0.7, 1.0, 0.7), floor(mod(mod_factor, 2.0)));

    FragColor = vec4(pow(mcol * multi, vec3(0.454545, 0.454545, 0.454545)), 1.0);
}

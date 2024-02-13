#version 450

/*
   MIT License
 
   Copyright (c) 2022 Filippo Scognamiglio
 
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
 
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
 
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/*
    Code ported by Hyllian from:
    https://github.com/Swordfish90/cheap-upscaling-triangulation
*/


layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float USE_DYNAMIC_SHARPNESS;
	float USE_SHARPENING_BIAS;
	float DYNAMIC_SHARPNESS_MIN;
	float DYNAMIC_SHARPNESS_MAX;
	float STATIC_SHARPNESS;
} params;

// set to 1.0 to use dynamic sharpening
#pragma parameter USE_DYNAMIC_SHARPNESS "Dynamic Sharpness [ 0FF | ON ]" 1.0 0.0 1.0 1.0

// Set to 1.0 to bias the interpolation towards sharpening
#pragma parameter USE_SHARPENING_BIAS "Sharpness Bias [ 0FF | ON ]" 0.0 0.0 1.0 1.0

// Minimum amount of sharpening in range [0.0, 1.0]
#pragma parameter DYNAMIC_SHARPNESS_MIN "Dynamic Sharpness Min" 0.0 0.0 1.0 0.1

// Maximum amount of sharpening in range [0.0, 1.0]
#pragma parameter DYNAMIC_SHARPNESS_MAX "Dynamic Sharpness Max" 0.8 0.0 1.0 0.1

// If USE_DYNAMIC_SHARPNESS is 0 apply this static sharpness
#pragma parameter STATIC_SHARPNESS "Static Sharpness" 0.2 0.0 1.0 0.1

#define USE_DYNAMIC_SHARPNESS  params.USE_DYNAMIC_SHARPNESS
#define USE_SHARPENING_BIAS    params.USE_SHARPENING_BIAS
#define DYNAMIC_SHARPNESS_MIN  (params.DYNAMIC_SHARPNESS_MIN * 0.5)
#define DYNAMIC_SHARPNESS_MAX  (params.DYNAMIC_SHARPNESS_MAX * 0.5)
#define STATIC_SHARPNESS       (params.STATIC_SHARPNESS * 0.5)

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;


float luma(vec3 v)
{
    return v.g;
}

float linearStep(float edge0, float edge1, float t)
{
    return clamp((t - edge0) / (edge1 - edge0), 0.0, 1.0);
}

float sharpSmooth(float t, float sharpness)
{
    return linearStep(sharpness, 1.0 - sharpness, t);
}

vec3 quadBilinear(vec3 a, vec3 b, vec3 c, vec3 d, vec2 p, float sharpness)
{
    float x = sharpSmooth(p.x, sharpness);
    float y = sharpSmooth(p.y, sharpness);
    return mix(mix(a, b, x), mix(c, d, x), y);
}

// Fast computation of barycentric coordinates only in the sub-triangle 1 2 4
vec3 fastBarycentric(vec2 p, float sharpness)
{
    float l0 = sharpSmooth(1.0 - p.x - p.y, sharpness);
    float l1 = sharpSmooth(p.x, sharpness);
    return vec3(l0, l1, 1.0 - l0 - l1);
}

vec3 triangleInterpolate(vec3 t1, vec3 t2, vec3 t3, vec3 t4, vec2 c, float sharpness)
{
    // Alter colors and coordinates to compute the other triangle.
    bool altTriangle = 1.0 - c.x < c.y;
    vec3 cornerColor = altTriangle ? t3 : t1;
    vec2 triangleCoords = altTriangle ? vec2(1.0 - c.y, 1.0 - c.x) : c;
    vec3 weights = fastBarycentric(triangleCoords, sharpness);
    return weights.x * cornerColor + weights.y * t2 + weights.z * t4;
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 screenCoords;

void main()
{
      gl_Position = global.MVP * Position;
      vTexCoord = TexCoord * 1.000001;
     
      screenCoords = vTexCoord*params.SourceSize.xy - vec2(0.5);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 screenCoords;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    float lmax, lmin, contrast;
    float sharpness = STATIC_SHARPNESS;

    vec2 relativeCoords = floor(screenCoords);
    vec2 c1 = ((relativeCoords + vec2(0.0, 0.0)) + vec2(0.5)) / params.SourceSize.xy;
    vec2 c2 = ((relativeCoords + vec2(1.0, 0.0)) + vec2(0.5)) / params.SourceSize.xy;
    vec2 c3 = ((relativeCoords + vec2(1.0, 1.0)) + vec2(0.5)) / params.SourceSize.xy;
    vec2 c4 = ((relativeCoords + vec2(0.0, 1.0)) + vec2(0.5)) / params.SourceSize.xy;

    vec3 t1 = texture(Source, c1).rgb;
    vec3 t2 = texture(Source, c2).rgb;
    vec3 t3 = texture(Source, c3).rgb;
    vec3 t4 = texture(Source, c4).rgb;

    float l1 = luma(t1);
    float l2 = luma(t2);
    float l3 = luma(t3);
    float l4 = luma(t4);

    if (USE_DYNAMIC_SHARPNESS == 1.0)
    {
        lmax = max(max(l1, l2), max(l3, l4));
        lmin = min(min(l1, l2), min(l3, l4));
        contrast = (lmax - lmin) / (lmax + lmin + 0.05);

        if (USE_SHARPENING_BIAS == 1.0)
            contrast = sqrt(contrast);

        sharpness = mix(DYNAMIC_SHARPNESS_MIN, DYNAMIC_SHARPNESS_MAX, contrast);
    }

    vec2 pxCoords = fract(screenCoords);

    float diagonal1Strength = abs(l1 - l3);
    float diagonal2Strength = abs(l2 - l4);

    // Alter colors and coordinates to compute the other triangulation.
    bool altTriangulation = diagonal1Strength < diagonal2Strength;

    vec3 cd = triangleInterpolate(
                                  altTriangulation ? t2 : t1,
                                  altTriangulation ? t3 : t2,
                                  altTriangulation ? t4 : t3,
                                  altTriangulation ? t1 : t4,
                                  altTriangulation ? vec2(pxCoords.y, 1.0 - pxCoords.x) : pxCoords,
                                  sharpness
                                 );

    float minDiagonal = min(diagonal1Strength, diagonal2Strength);
    float maxDiagonal = max(diagonal1Strength, diagonal2Strength);
    bool diagonal     = minDiagonal * 4.0 + 0.05 < maxDiagonal;

    vec3 final = diagonal ? cd : quadBilinear(t1, t2, t4, t3, pxCoords, sharpness);

    FragColor = vec4(final, 1.0);
}

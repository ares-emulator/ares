#version 450

// OmniScale-Legacy
// by Lior Halphon
// ported to slang format by hunterk

/*
MIT License

Copyright (c) 2015-2016 Lior Halphon

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

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

float quickDistance(vec4 a, vec4 b)
{
    return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
}

#define uResolution params.OutputSize.xy
#define textureDimensions params.SourceSize.xy

vec4 omniScale(sampler2D image, vec2 texCoord)
{
    vec2 pixel = texCoord * textureDimensions - vec2(0.5, 0.5);

    vec4 q11 = texture(image, vec2(floor(pixel.x) / textureDimensions.x, floor(pixel.y) / textureDimensions.y+0.001));
    vec4 q12 = texture(image, vec2(floor(pixel.x) / textureDimensions.x, ceil(pixel.y) / textureDimensions.y+0.001));
    vec4 q21 = texture(image, vec2(ceil(pixel.x) / textureDimensions.x, floor(pixel.y) / textureDimensions.y+0.001));
    vec4 q22 = texture(image, vec2(ceil(pixel.x) / textureDimensions.x, ceil(pixel.y) / textureDimensions.y+0.001));

    vec2 pos = fract(pixel);

    /* Special handling for diaonals */
    bool hasDownDiagonal = false;
    bool hasUpDiagonal = false;
    if (q12 == q21 && q11 != q22) hasUpDiagonal = true;
    else if (q12 != q21 && q11 == q22) hasDownDiagonal = true;
    else if (q12 == q21 && q11 == q22) {
        if (q11 == q12) return q11;
        int diagonalBias = 0;
        for (float y = -1.0; y < 3.0; y++) {
            for (float x = -1.0; x < 3.0; x++) {
                vec4 color = texture(image, (pixel + vec2(x, y)) / textureDimensions);
                if (color == q11) diagonalBias++;
                if (color == q12) diagonalBias--;
            }
        }
        if (diagonalBias <= 0) {
            hasDownDiagonal = true;
        }
        if (diagonalBias >= 0) {
            hasUpDiagonal = true;
        }
    }

    if (hasUpDiagonal || hasDownDiagonal) {
        vec4 downDiagonalResult, upDiagonalResult;

        if (hasUpDiagonal) {
            float diagonalPos = pos.x + pos.y;

            if (diagonalPos < 0.5) {
                upDiagonalResult = q11;
            }
            else if (diagonalPos > 1.5) {
                upDiagonalResult = q22;
            }
            else {
                upDiagonalResult = q12;
            }
        }

        if (hasDownDiagonal) {
            float diagonalPos = 1.0 - pos.x + pos.y;

            if (diagonalPos < 0.5) {
                downDiagonalResult = q21;
            }
            else if (diagonalPos > 1.5) {
                downDiagonalResult = q12;
            }
            else {
                downDiagonalResult = q11;
            }
        }

        if (!hasUpDiagonal) return downDiagonalResult;
        if (!hasDownDiagonal) return upDiagonalResult;
        return mix(downDiagonalResult, upDiagonalResult, 0.5);
    }

    vec4 r1 = mix(q11, q21, fract(pos.x));
    vec4 r2 = mix(q12, q22, fract(pos.x));

    vec4 unquantized = mix(r1, r2, fract(pos.y));

    float q11d = quickDistance(unquantized, q11);
    float q21d = quickDistance(unquantized, q21);
    float q12d = quickDistance(unquantized, q12);
    float q22d = quickDistance(unquantized, q22);

    float best = min(q11d,
                     min(q21d,
                         min(q12d,
                             q22d)));

    if (q11d == best) {
        return q11;
    }

    if (q21d == best) {
        return q21;
    }
    
    if (q12d == best) {
        return q12;
    }
    
    return q22;
}

vec4 scale(sampler2D tex, vec2 coord){
    vec2 texCoord = coord;
    vec2 pixel = vec2(1.0, 1.0) / uResolution;
    // 4-pixel super sampling

    vec4 q11 = omniScale(tex, texCoord + pixel * vec2(-0.25, -0.25));
    vec4 q21 = omniScale(tex, texCoord + pixel * vec2(+0.25, -0.25));
    vec4 q12 = omniScale(tex, texCoord + pixel * vec2(-0.25, +0.25));
    vec4 q22 = omniScale(tex, texCoord + pixel * vec2(+0.25, +0.25));
	
	return (q11 + q21 + q12 + q22) / 4.0;
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    FragColor = scale(Source, vTexCoord);
}

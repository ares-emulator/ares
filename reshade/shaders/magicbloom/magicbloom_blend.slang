#version 450

/*
    Magic Bloom by luluco250
    Attempts to simulate a natural-looking bloom.
    Features:
    --Wide bloom blurring, derived from the gaussian function
      defined here: https://en.wikipedia.org/wiki/Gaussian_blur#Mathematics
    --Eye adaptation, decreases or increases the brightness 
      of bloom according to the overall image luminance.
    --Lens dirt, as standard I suppose. Really not much here. 
      It uses an image named "MagicBloom_Dirt.png" so make 
      sure you have one in your textures directory.
    --Unwanted features can be disabled through 
      preprocessor definitions, saving performance.
    
    Preprocessor definitions:
    --MAGICBLOOM_ADAPT_RESOLUTION:
        Determines the width/height of the texture used for adaptation.
        It is recommended to use 256, but you can use as far as 1024 without issues.
        Too low resolutions will make adaptation seem "unstable".
        Must be a power of two value: 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 etc.
    --MAGICBLOOM_BLUR_PRECALCULATED:
        If set to 0 the gaussian blur will be calculated inside the shader.
        Otherwise, it uses a pre-calculated kernel (array).
    
    --MAGICBLOOM_NODIRT:
        If set to 1 all lens dirt related features are disabled.
        Beneficial for performance if you don't wish to use lens dirt.
    --MAGICBLOOM_NOADAPT:
        If set to 1 all adaptation related features are disabled.
        Beneficial for performance if you don't wish to use adaptation.
    MIT Licensed:
    Copyright (c) 2017 luluco250
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

//Statics

#ifndef MAGICBLOOM_ADAPT_RESOLUTION
    #define MAGICBLOOM_ADAPT_RESOLUTION 256
#endif

#ifndef MAGICBLOOM_BLUR_PRECALCULATED
    #define MAGICBLOOM_BLUR_PRECALCULATED 1
#endif

#ifndef MAGICBLOOM_NODIRT
    #define MAGICBLOOM_NODIRT 0
#endif

// #ifndef MAGICBLOOM_NOADAPT
    #define MAGICBLOOM_NOADAPT 0
// #endif

#define iBlurSamples 4
float iAdaptResolution = MAGICBLOOM_ADAPT_RESOLUTION;

// #define CONST_LOG2(v) (((v) & 0xAAAAAAAA) != 0) | ((((v) & 0xFFFF0000) != 0) << 4) | ((((v) & 0xFF00FF00) != 0) << 3) | ((((v) & 0xF0F0F0F0) != 0) << 2) | ((((v) & 0xCCCCCCCC) != 0) << 1)
#define CONST_LOG2(v) log2(v)

int sigma = int(float(iBlurSamples) / 2.0);
float double_pi = 6.283185307179586476925286766559;
float lowest_mip = CONST_LOG2(iAdaptResolution) + 1;
vec3 luma_value = vec3(0.2126, 0.7152, 0.0722);


layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
    float ApplyBloom;
    float fExposure;
    float iDebug;
    float fDirt_Intensity;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter ApplyBloom        "          Apply Bloom" 1 0 1 1
#define ApplyBloom params.ApplyBloom

#pragma parameter fExposure         "          Exposure" 0.2 0 1 0.01
#define fExposure params.fExposure

#pragma parameter fDirt_Intensity   "          Dirt Intensity" 0 0 1 0.01
#define fDirt_Intensity params.fDirt_Intensity

#pragma parameter iDebug            "          Debug: Show Bloom Contribution" 0 0 1 1
#define iDebug params.iDebug

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.00001;
}

#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D PreBloomPass;
layout(set = 0, binding = 4) uniform sampler2D tMagicBloom_1;
layout(set = 0, binding = 5) uniform sampler2D tMagicBloom_2;
layout(set = 0, binding = 6) uniform sampler2D tMagicBloom_3;
layout(set = 0, binding = 7) uniform sampler2D tMagicBloom_4;
layout(set = 0, binding = 8) uniform sampler2D tMagicBloom_5;
layout(set = 0, binding = 9) uniform sampler2D tMagicBloom_6;
layout(set = 0, binding = 10) uniform sampler2D tMagicBloom_7;
layout(set = 0, binding = 11) uniform sampler2D tMagicBloom_8;
layout(set = 0, binding = 12) uniform sampler2D tMagicBloom_Adapt;
layout(set = 0, binding = 13) uniform sampler2D tMagicBloom_Dirt;

//Functions

/*
    Uncharted 2 Tonemapper
    Thanks John Hable and Naughty Dog.
*/
vec3 tonemap(vec3 col, float exposure) {
    const float A = 0.15; //shoulder strength
    const float B = 0.50; //linear strength
	const float C = 0.10; //linear angle
	const float D = 0.20; //toe strength
	const float E = 0.02; //toe numerator
	const float F = 0.30; //toe denominator
	const float W = 11.2; //linear white point value

    col *= exposure;

    col = ((col * (A * col + C * B) + D * E) / (col * (A * col + B) + D * F)) - E / F;
    const float white = 1.0 / (((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F);
    col *= white;
    return col;
}

vec3 blend_screen(vec3 a, vec3 b) {
    return 1.0 - (1.0 - a) * (1.0 - b);
}

//Final blend shader
vec4 PS_Blend(vec2 uv){
    vec3 col = texture(PreBloomPass, uv).rgb;
    vec3 bloom = 0
                 + texture(tMagicBloom_1, uv).rgb
                 + texture(tMagicBloom_2, uv).rgb
                 + texture(tMagicBloom_3, uv).rgb
                 + texture(tMagicBloom_4, uv).rgb
                 + texture(tMagicBloom_5, uv).rgb
                 + texture(tMagicBloom_6, uv).rgb
                 + texture(tMagicBloom_7, uv).rgb
                 + texture(tMagicBloom_8, uv).rgb;
    bloom /= 8;

    // TODO need to use the feedback, also the blend is possibly the last pass
#if !MAGICBLOOM_NOADAPT
    float exposure = fExposure / max(texture(tMagicBloom_Adapt, vec2(0.0)).x, 0.00001);
    bloom = tonemap(bloom, exposure);
#else
    // Without adaptation it seems 100.0 exposure is needed for bloom to look bright enough.
    bloom = tonemap(bloom, 100.0);
#endif

    #if !MAGICBLOOM_NODIRT
    vec3 dirt = texture(tMagicBloom_Dirt, uv).rgb;
    dirt *= fDirt_Intensity;
    bloom = blend_screen(bloom, dirt * bloom);
    #endif

    vec3 col_max = max(col, bloom);

    col = blend_screen(col, bloom);

    // If we're to display the bloom texture, we replace col with it.
    col = iDebug == 1 ? bloom : col;

    return vec4(col, 1.0);
}

void main()
{
    if (ApplyBloom < 1){
        FragColor = texture(PreBloomPass, vTexCoord);
        return;
    }

    FragColor = PS_Blend(vTexCoord);
}

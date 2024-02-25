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

#pragma format R16G16B16A16_SFLOAT

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

#ifndef MAGICBLOOM_NOADAPT
    #define MAGICBLOOM_NOADAPT 0
#endif

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
    float fBloom_Intensity;
    float fBloom_Threshold;
    float ApplyBloom;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter MagicBloomTitle "[ MAGIC BLOOM ]" 0 0 0.001 0.001

#pragma parameter ApplyBloom "          Apply Bloom" 1 0 1 1
#define ApplyBloom params.ApplyBloom

#pragma parameter fBloom_Intensity "          Bloom Intensity" 1 0 10 0.1
#define fBloom_Intensity params.fBloom_Intensity / 4

#pragma parameter fBloom_Threshold "          Bloom Threshold" 2 1 10 0.1
#define fBloom_Threshold params.fBloom_Threshold


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

//Functions

#if !MAGICBLOOM_BLUR_PRECALCULATED
float gaussian_function(vec2 i) {
    float first_part = 1.0 / (double_pi * pow(sigma, 2.0));
    float second_part_a = 1.0 / (2.0 * pow(sigma, 2.0));
    float second_part_b = (pow(i.x, 2.0) + pow(i.y, 2.0)) * second_part_a;
    return first_part * exp(-second_part_b);
}
#endif

//Why use a single-pass blur? To reduce the amount of textures used in half.
//Scale should be the original resolution divided by target resolution.
vec3 blur(sampler2D sp, vec2 uv, float scale) {
    vec2 ps = params.OutputSize.zw * scale;
    
    #if MAGICBLOOM_BLUR_PRECALCULATED
    float kernel[9] = { 
        0.0269955, 0.0647588, 0.120985, 0.176033, 0.199471, 0.176033, 0.120985, 0.0647588, 0.0269955 
    };
    float accum = 1.02352;
    #else
    float accum = 0.0;
    #endif
    
    float gaussian_weight = 0.0;
    vec3 col = vec3(0);
    
    // [unroll]
    for (int x = int(-iBlurSamples); x <= int(iBlurSamples); ++x) {
        for (int y = int(-iBlurSamples); y <= int(iBlurSamples); ++y) {
            #if MAGICBLOOM_BLUR_PRECALCULATED
            gaussian_weight = kernel[x + iBlurSamples] * kernel[y + iBlurSamples];
            #else
            gaussian_weight = gaussian_function(vec2(x, y));
            accum += gaussian_weight;
            #endif
            col += texture(sp, uv + ps * vec2(x, y)).rgb * gaussian_weight;
        }
    }

    #if MAGICBLOOM_BLUR_PRECALCULATED
    return col * accum;
    #else
    return col / accum;
    #endif
}

vec4 PS_Blur1(vec2 uv, sampler2D in_sampler) {
    vec3 col = blur(in_sampler, uv, 2.0);
    col = pow(abs(col), vec3(fBloom_Threshold));
    col *= fBloom_Intensity;
    return vec4(col, 1.0);
}

vec4 PS_Blur2(vec2 uv, sampler2D in_sampler) {
    return vec4(blur(in_sampler, uv, 4.0), 1.0);
}

vec4 PS_Blur3(vec2 uv, sampler2D in_sampler) {
    return vec4(blur(in_sampler, uv, 8.0), 1.0);
}

vec4 PS_Blur4(vec2 uv, sampler2D in_sampler) {
    return vec4(blur(in_sampler, uv, 8.0), 1.0);
}

vec4 PS_Blur5(vec2 uv, sampler2D in_sampler) {
    return vec4(blur(in_sampler, uv, 16.0), 1.0);
}

vec4 PS_Blur6(vec2 uv, sampler2D in_sampler) {
    return vec4(blur(in_sampler, uv, 32.0), 1.0);
}

vec4 PS_Blur7(vec2 uv, sampler2D in_sampler) {
    return vec4(blur(in_sampler, uv, 64.0), 1.0);
}

vec4 PS_Blur8(vec2 uv, sampler2D in_sampler) {
    return vec4(blur(in_sampler, uv, 128.0), 1.0);
}

void main()
{
    if (ApplyBloom < 1)
        return;

#ifdef BLUR_PASS_1
    FragColor = PS_Blur1(vTexCoord, Source);
#endif

#ifdef BLUR_PASS_2
    FragColor = PS_Blur2(vTexCoord, Source);
#endif

#ifdef BLUR_PASS_3
    FragColor = PS_Blur3(vTexCoord, Source);
#endif

#ifdef BLUR_PASS_4
    FragColor = PS_Blur4(vTexCoord, Source);
#endif

#ifdef BLUR_PASS_5
    FragColor = PS_Blur5(vTexCoord, Source);
#endif

#ifdef BLUR_PASS_6
    FragColor = PS_Blur6(vTexCoord, Source);
#endif

#ifdef BLUR_PASS_7
    FragColor = PS_Blur7(vTexCoord, Source);
#endif

#ifdef BLUR_PASS_8
    FragColor = PS_Blur8(vTexCoord, Source);
#endif

}

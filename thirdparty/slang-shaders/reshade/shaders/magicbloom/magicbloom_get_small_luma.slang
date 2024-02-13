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

#pragma format R32G32B32A32_SFLOAT

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
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter ApplyBloom "          Apply Bloom" 1 0 1 1
#define ApplyBloom params.ApplyBloom

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
layout(set = 0, binding = 2) uniform sampler2D PreBloomPass;

//Functions

float PS_GetSmall(vec2 uv){
    return dot(texture(PreBloomPass, uv).rgb, luma_value);
}

void main()
{
    if (ApplyBloom < 1)
        return;

    FragColor = vec4(PS_GetSmall(vTexCoord));
}

#version 450

//////////////////////////////////////////////////////////////////////////
//
// CC0 1.0 Universal (CC0 1.0)
// Public Domain Dedication 
//
// To the extent possible under law, J. Kyle Pittman has waived all
// copyright and related or neighboring rights to this implementation
// of CRT simulation. This work is published from the United States.
//
// For more information, please visit
// https://creativecommons.org/publicdomain/zero/1.0/
//
//////////////////////////////////////////////////////////////////////////

// Multi-purpose code used for both downsampling and upsampling the full-resolution output image.

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float bloom_scale_up;
} params;

#pragma parameter bloom_scale_up "Upsample Bloom Scale" 0.02 0.0 0.03 0.001

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define half4 vec4
#define half3 vec3
#define half2 vec2
#define half float
#define float2 vec2
#define lerp(a, b, c) mix(a, b, c)
#define tex2D(a, b) texture(a, b)
#define mul(a, b) (b * a)
#define saturate(c) clamp(c, 0.0, 1.0)

const float2 Poisson0 = float2(0.000000, 0.000000);
const float2 Poisson1 = float2(0.000000, 1.000000);
const float2 Poisson2 = float2(0.000000, -1.000000);
const float2 Poisson3 = float2(-0.866025, 0.500000);
const float2 Poisson4 = float2(-0.866025, -0.500000);
const float2 Poisson5 = float2(0.866025, 0.500000);
const float2 Poisson6 = float2(0.866025, -0.500000);

const float InvNumSamples = 0.1428571428571429; //<- 1.0 / number of Poisson samples

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
#define DownsampleBufferSampler Source

void main()
{
	vec4 bloom = vec4(0.0);
	vec2 BloomScale = vec2(params.bloom_scale_up);
	bloom += tex2D(DownsampleBufferSampler, vTexCoord + (Poisson0.yx * BloomScale));
	bloom += tex2D(DownsampleBufferSampler, vTexCoord + (Poisson1.yx * BloomScale));
	bloom += tex2D(DownsampleBufferSampler, vTexCoord + (Poisson2.yx * BloomScale));
	bloom += tex2D(DownsampleBufferSampler, vTexCoord + (Poisson3.yx * BloomScale));
	bloom += tex2D(DownsampleBufferSampler, vTexCoord + (Poisson4.yx * BloomScale));
	bloom += tex2D(DownsampleBufferSampler, vTexCoord + (Poisson5.yx * BloomScale));
	bloom += tex2D(DownsampleBufferSampler, vTexCoord + (Poisson6.yx * BloomScale));
	bloom *= InvNumSamples;

   FragColor = vec4(bloom);
}
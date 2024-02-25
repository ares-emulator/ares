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

// Blends the original full-resolution scene with the blurred output of post shaders to create bloom.

#pragma parameter BloomPower "Bloom Power" 1.0 0.0 10.0 0.1
#pragma parameter BloomScalar "Bloom Scalar" 0.1 0.0 1.0 0.05

#pragma parameter Tuning_Overscan "Overscan" 0.95 0.0 1.0 0.05
#pragma parameter Tuning_Barrel "Barrel Distortion" 0.25 0.0 1.0 0.05
#pragma parameter mask_toggle "Mask Toggle" 1.0 0.0 1.0 1.0
//#pragma parameter mixfactor "Bloom Mix" 0.5 0.0 1.0 0.05

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float BloomPower;
	float BloomScalar;
	float Tuning_Overscan;
	float Tuning_Barrel;
	float mask_toggle;
//	float mixfactor;
} global;

#define half4 vec4
#define half3 vec3
#define half2 vec2
#define half float
#define lerp(a, b, c) mix(a, b, c)
#define tex2D(a, b) texture(a, b)
#define mul(a, b) (b * a)
#define saturate(c) clamp(c, 0.0, 1.0)

#include "crtbase.h"

// Apply power to brightness while preserving color
// TODO: Clamp ActLuma to very small number to prevent (zero) division by zero when a component is zero?
half4 ColorPow(half4 InColor, half InPower)
{
	// This method preserves color better.
	half4 RefLuma = half4(0.299, 0.587, 0.114, 0.0);
	half ActLuma = dot(InColor, RefLuma);
	half4 ActColor = InColor / ActLuma;
	half PowLuma = pow(ActLuma, InPower);
	half4 PowColor = ActColor * PowLuma;
	return PowColor;
}

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
layout(set = 0, binding = 3) uniform sampler2D CRTPASS;
layout(set = 0, binding = 4) uniform sampler2D shadowMaskSampler;

void main()
{
	// Apply overscan after scanline sampling is done.
	half2 overscanuv = (vTexCoord * global.Tuning_Overscan) - ((global.Tuning_Overscan - 1.0) * 0.5);
	
	// Curve UVs for composite texture inwards to garble things a bit.
	overscanuv = overscanuv - half2(0.5,0.5);
	half rsq = (overscanuv.x*overscanuv.x) + (overscanuv.y*overscanuv.y);
	overscanuv = overscanuv + (overscanuv * (global.Tuning_Barrel * rsq)) + half2(0.5,0.5);
	vec4 PreBloom = vec4(0.0);
	// Mask effect cancels curvature due to righteous moire
	overscanuv = (global.mask_toggle > 0.5) ? vTexCoord : overscanuv;
	PreBloom = (global.mask_toggle > 0.5) ? SampleCRT(shadowMaskSampler, CRTPASS, overscanuv) : texture(CRTPASS, overscanuv);

	vec4 Blurred = texture(Source, overscanuv);
   FragColor = vec4(PreBloom + (ColorPow(Blurred, global.BloomPower) * global.BloomScalar));//vec4(mix(PreBloom, Blurred, global.mixfactor));
}
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

#define half4 vec4
#define half3 vec3
#define half2 vec2
#define half float
#define lerp(a, b, c) mix(a, b, c)
#define tex2D(a, b) texture(a, b)
#define mul(a, b) (b * a)
#define saturate(c) clamp(c, 0.0, 1.0)

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float FrameColor_R;
	float FrameColor_G;
	float FrameColor_B;
} global;

#pragma parameter FrameColor_R "Frame Color R" 1.0 0.0 1.0 0.05
#pragma parameter FrameColor_G "Frame Color G" 1.0 0.0 1.0 0.05
#pragma parameter FrameColor_B "Frame Color B" 1.0 0.0 1.0 0.05

#include "crtbase.h"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
//layout(location = 1) out vec3 norm;
//layout(location = 2) out vec3 camDir;
//layout(location = 3) out vec3 lightDir;
//layout(location = 4) out float blend;

//mat4x4 wvpMat = global.MVP;
//mat4x4 worldMat = global.MVP;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   /*
   // This is really unnecessary since it SHOULD always be identity, but whatever...
	half3 worldPos = mul(half4(Position.xyz, 1.0), worldMat).xyz;
	
	// Don't normalize this pre-pixel shader
	camDir = camPos - worldPos;
	lightDir = params.Tuning_LightPos - worldPos;
	*/
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
//layout(location = 1) in vec3 norm;
//layout(location = 2) in vec3 camDir;
//layout(location = 3) in vec3 lightDir;
//layout(location = 4) in float blend;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D shadowMaskSampler;

void main()
{
	/*
	half3 norm = normalize(norm);
	
	half3 camDir = normalize(camDir);
	half3 lightDir = normalize(lightDir);
	
	half diffuse = saturate(dot(norm, lightDir));
	
	half3 worldUp = half3(0.,0.,1.);
	half hemi = dot(norm, worldUp) * 0.5 + 0.5;
	hemi = hemi * 0.4 + 0.3;			// [0.3,0.7] range
	
	vec4 Tuning_FrameColor = vec4(global.FrameColor_R, global.FrameColor_G, global.FrameColor_B, 1.0);
	half4 colordiff = Tuning_FrameColor * (diffuse + hemi) * params.Tuning_Diff_Brightness;
	
	half3 halfVec = normalize(lightDir + camDir);
	half spec = saturate(dot(norm, halfVec));
	spec = pow(spec, Tuning_Spec_Power);
	half4 colorspec = half4(0.25, 0.25, 0.25, 1.) * spec * params.Tuning_Spec_Brightness;
	
	half4 emissive = SampleCRT(shadowMaskSampler, Source, vTexCoord);
	
	colorspec += (emissive * blend * params.Tuning_ReflScalar);
	
	half fres = 1.0 - dot(camDir, norm);
	fres = (fres*fres) * params.Tuning_Fres_Brightness;
	half4 colorfres = half4(0.15, 0.15, 0.15, 1.) * fres;
	
	half4 nearfinal = (colorfres + colordiff + colorspec);
	
	FragColor = (nearfinal * lerp(ivec4(1,1,1,1), color, Tuning_Dimming));
	*/
   FragColor = vec4(SampleCRT(shadowMaskSampler, Source, vTexCoord));
}
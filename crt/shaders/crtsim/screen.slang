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
} global;

#include "crtbase.h"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
//layout(location = 1) out vec3 norm;
//layout(location = 2) out vec3 camDir;
//layout(location = 3) out vec3 lightDir;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
	
/*
//	Going head and getting this stuff ready but not hooking anything up yet
	mat3x3 wMat3 = mat3x3(worldMat[0].xyz, worldMat[1].xyz, worldMat[2].xyz);
	mat3x3 invWorldRot = transpose(wMat3);
	
	// Don't normalize this pre-pixel shader
	camDir = mul(camPos - worldPos, invWorldRot);
	vec3 Tuning_lightPos = vec3(params.Tuning_lightPos_R, params.Tuning_lightPos_G, params.Tuning_lightPos_B);
	lightDir = mul(Tuning_LightPos - worldPos, invWorldRot)
*/
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
//layout(location = 1) in vec3 norm;
//layout(location = 2) in vec3 camDir;
//layout(location = 3) in vec3 lightDir;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D shadowMaskSampler;

void main()
{
	/*
	half3 norm = normalize(norm);
	
	half3 camDir = normalize(camDir);
	half3 lightDir = normalize(lightDir);
	
	half3 refl = reflect(camDir, norm);
		
	half diffuse = saturate(dot(norm, lightDir));
	half4 colordiff = half4(0.175, 0.15, 0.2, 1.) * diffuse * params.Tuning_Diff_Brightness;
	
	half3 halfVec = normalize(lightDir + camDir);
	half spec = saturate(dot(norm, halfVec));
	spec = pow(spec, params.Tuning_Spec_Power);
	half4 colorspec = half4(0.25, 0.25, 0.25, 1.) * spec * params.Tuning_Spec_Brightness;
	
	half fres = 1.0 - dot(camDir, norm);
	fres = (fres*fres) * params.Tuning_Fres_Brightness;
	half4 colorfres = half4(0.45, 0.4, 0.5, 1.) * fres;
	
	half4 emissive = SampleCRT(shadowMaskSampler, Source, vTexCoord);
	
	half4 nearfinal = colorfres + colordiff + colorspec + emissive;
	
	FragColor = (nearfinal * lerp(ivec4(1,1,1,1), color, params.Tuning_Dimming)); //TODO: not sure what 'color' would be here
	*/
   FragColor = vec4(SampleCRT(shadowMaskSampler, Source, vTexCoord));
}
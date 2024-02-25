#version 450

/*
   AGS-001 shader
   A pristine recreation of the illuminated Game Boy Advance SP
   Author: endrift
   License: MPL 2.0

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. 
*/

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
} global;

layout(push_constant) uniform Push
{
	float reflectionBrightness;
	float reflectionDistanceX;
	float reflectionDistanceY;
	float lightBrightness;
}params;

#pragma parameter reflectionBrightness "Reflection brightness" 0.07 0.0 1.0 0.01
#pragma parameter reflectionDistanceX "Reflection Distance X" 0.0 -1.0 1.0 0.005
#pragma parameter reflectionDistanceY "Reflection Distance Y" 0.025 -1.0 1.0 0.005
#pragma parameter lightBrightness "Light brightness" 1.0 0.0 1.0 0.01

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

const float speed = 2.0;
const float decay = 2.0;
const float coeff = 2.5;

void main()
{
	vec2 reflectionDistance = vec2(params.reflectionDistanceX,params.reflectionDistanceY);
	float sp = pow(speed, params.lightBrightness);
	float dc = pow(decay, -params.lightBrightness);
	float s = (sp - dc) / (sp + dc);
	vec2 radius = (vTexCoord.st - vec2(0.5, 0.5)) * vec2(coeff * s);
	radius = pow(abs(radius), vec2(4.0));
	vec3 bleed = vec3(0.12, 0.14, 0.19);
	bleed += (dot(radius, radius) + vec3(0.02, 0.03, 0.05)) * vec3(0.14, 0.18, 0.2);

	vec4 color = texture(Source, vTexCoord);
	color.rgb += pow(bleed, pow(vec3(params.lightBrightness), vec3(-0.5)));

	vec4 reflection = texture(Source, vTexCoord - reflectionDistance);
	color.rgb += reflection.rgb * params.reflectionBrightness;
	color.a = 1.0;
	FragColor = color;

}

#version 450

/*
   Super-res shader - Shiandow

   Ported from Shiandow's code by Hyllian, 2016.

   This file is a part of MPDN Extensions.
   https://github.com/zachsaw/MPDN_Extensions
  
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3.0 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library.
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
layout(set = 0, binding = 3) uniform sampler2D diff_refpass;

const vec3 Y = vec3(.2126, .7152, .0722);


float RGBtoYUV(vec3 color)
{
  return dot(color, Y);
}

void main()
{
	vec2 tex = vTexCoord;

	vec4 c0 = texture(Source, tex);
	vec4 c1 = texture(diff_refpass, tex);

	FragColor = vec4(c0.xyz - c1.xyz, RGBtoYUV(c0.rgb));
}

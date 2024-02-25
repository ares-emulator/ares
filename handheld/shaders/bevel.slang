#version 450

/*
   Hyllian's Bevel Shader
  
   Copyright (C) 2011-2014 Hyllian/Jararaca - sergiogdb@gmail.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float BEVEL_LEVEL;
} params;

#pragma parameter BEVEL_LEVEL "Bevel Level" 0.2 0.0 0.5 0.01
#define BEVEL_LEVEL params.BEVEL_LEVEL

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// Constants used with gamma correction.
#define InputGamma             2.4
#define OutputGamma            2.2

#define GAMMA_IN(color)     pow(color, vec3(InputGamma, InputGamma, InputGamma))
#define GAMMA_OUT(color)    pow(color, vec3(1.0 / OutputGamma, 1.0 / OutputGamma, 1.0 / OutputGamma))

vec3 bevel(vec2 pos, vec3 color)
{
    vec3 weight;

    float r = sqrt(dot(pos, vec2(1.0)));

    vec3 delta = mix(vec3(BEVEL_LEVEL), vec3(1.0-BEVEL_LEVEL), color);

    weight = delta*(1-r);

    return color+weight;
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

void main()
{
    vec2 position = fract(vTexCoord*params.SourceSize.xy);

    vec3 color = GAMMA_IN(texture(Source, vTexCoord).rgb);

    color = clamp(bevel(position, color), 0.0, 1.0);

   FragColor = vec4(GAMMA_OUT(color), 1.0);
}
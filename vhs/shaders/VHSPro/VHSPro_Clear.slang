#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#define SamplerColorVHS Source

#include "VHSPro_params.inc"
#include "VHSPro_constants.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 txcoord;

void main()
{
   gl_Position = global.MVP * Position;
   txcoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 txcoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#include "VHSPro_functions.inc"

void main()
{
   FragColor = vec4(texture(SamplerColorVHS, txcoord).rgb, 1.0);
}
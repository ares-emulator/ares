#version 450

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

#include "hsm-crt-easymode-blur_params.inc"

#define kernel(x) exp(-GLOW_FALLOFF * (x) * (x))

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
	vec3 col = vec3(0.0);
	float dx = params.SourceSize.z;

	float k_total = 0.0;
	for (int i = -TAPS; i <= TAPS; i++)
		{
		float k = kernel(i);
		k_total += k;
		col += k * texture(Source, vTexCoord + vec2(float(i) * dx, 0.0)).rgb;
		}
   FragColor = vec4(col / k_total, 1.0);
}

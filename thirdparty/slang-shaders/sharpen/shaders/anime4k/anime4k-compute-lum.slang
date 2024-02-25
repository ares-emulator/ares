#version 450

/*
	Anime4k - Luma shader - ported by Hyllian - 2020
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

void main()
{
	vec4 c0 = texture(Source, vTexCoord);
	
	//Quick luminance approximation
	float lum = (c0.r + c0[0].r + c0.g + c0.g + c0.g + c0.b) / 6;
	
	//Computes the luminance and saves it in the unused alpha channel
	FragColor = vec4(c0.r, c0.g, c0.b, lum);
}
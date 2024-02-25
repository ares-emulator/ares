#version 450

// Implementation based on the article "Efficient Gaussian blur with linear sampling"
// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
/* A version for MasterEffect Reborn, a standalone version, and a custom shader version for SweetFX can be 
   found at http://reshade.me/forum/shader-presentation/27-gaussian-blur-bloom-unsharpmask */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float BloomIntensity;
} params;

#pragma parameter BloomIntensity "BloomIntensity" 3.0 1.0 4.0 0.1
#define BloomIntensity params.BloomIntensity

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
	vec4 color = texture(Source, vTexCoord);
   FragColor = vec4(color.rgb * pow (abs (max (color.r, max (color.g, color.b))), 2.0), 2.0f)*BloomIntensity;
}
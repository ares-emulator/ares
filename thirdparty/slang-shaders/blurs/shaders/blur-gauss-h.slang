#version 450

// Implementation based on the article "Efficient Gaussian blur with linear sampling"
// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
/* A version for MasterEffect Reborn, a standalone version, and a custom shader version for SweetFX can be 
   found at http://reshade.me/forum/shader-presentation/27-gaussian-blur-bloom-unsharpmask */
 /*-----------------------------------------------------------.
/                  Gaussian Blur settings                     /
'-----------------------------------------------------------*/

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

#define HW 1.00

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
	vec2 texcoord  = vTexCoord;
	vec2 PIXEL_SIZE = params.SourceSize.zw;

	float sampleOffsets[5] = { 0.0, 1.4347826, 3.3478260, 5.2608695, 7.1739130 };
	float sampleWeights[5] = { 0.16818994, 0.27276957, 0.11690125, 0.024067905, 0.0021112196 };

	vec4 color = texture(Source, texcoord) * sampleWeights[0];
	for(int i = 1; i < 5; ++i) {
		color += texture(Source, texcoord + vec2(sampleOffsets[i]*HW * PIXEL_SIZE.x, 0.0)) * sampleWeights[i];
		color += texture(Source, texcoord - vec2(sampleOffsets[i]*HW * PIXEL_SIZE.x, 0.0)) * sampleWeights[i];
	}
   FragColor = vec4(color);
}
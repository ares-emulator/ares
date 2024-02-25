#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float diffusion;
	float out_gamma;
} params;

#pragma parameter diffusion "Halation Strength" 0.5 0.0 1.0 0.01
#pragma parameter out_gamma "Display Gamma" 2.2 1.5 3.0 0.1

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
layout(set = 0, binding = 2) uniform sampler2D firstPass;
layout(set = 0, binding = 3) uniform sampler2D blurPass;
layout(set = 0, binding = 4) uniform sampler2D phosphorPass;
//layout(set = 0, binding = 5) uniform sampler2D blurPassV;

void main()
{
	vec3 scanlines = texture(firstPass, vTexCoord).rgb;
//	vec3 blurV = texture(blurPassV, vTexCoord).rgb;
	vec3 blurH = texture(blurPass, vTexCoord).rgb;
	vec3 blurLines = (scanlines + blurH) / 2.0;
	vec3 phosphors = texture(phosphorPass, vTexCoord).rgb;
	vec3 glow = (phosphors + blurH) / 2.0;
	vec3 halation = mix(blurLines, phosphors, params.diffusion);
	//vec3 halation = 1.0 - (1.0 - phosphors) * (1.0 - blurLines);
	halation = 1.0 - (1.0 - halation) * (1.0 - scanlines);
   FragColor = vec4(pow(halation, vec3(1.0 / params.out_gamma)), 1.0);
}
#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float InputGamma;
} params;

#pragma parameter InputGamma "INPUT GAMMA" 1.0 1.0 3.0 0.05

#define InputGamma params.InputGamma

#define GAMMA_IN(color)     pow(color, vec3(InputGamma, InputGamma, InputGamma))

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
//   FragColor = pow(vec4(texture(Source, vTexCoord).rgb, 1.0), vec4(2.2));
   FragColor = vec4(GAMMA_IN(texture(Source, vTexCoord).rgb), 1.0);
}

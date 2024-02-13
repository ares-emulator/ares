#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float InputGamma;
	float CD_BLEND_OPTION;
	float HSM_DEDITHER_MODE;
} params;

#pragma parameter HSM_DEDITHERING_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_DEDITHERING_TITLE "[ --- MDAPT & HYLLIAN DE-DITHERING --- ]:" 0 0 0.01 0.01

#pragma parameter HSM_DEDITHER_MODE "Mode: OFF| STRICT | RELAXED | HYLLIAN | HYL + STRIPES | STRIPES"	0 0 5 1
#define HSM_DEDITHER_MODE  params.HSM_DEDITHER_MODE

#define InputGamma 2.0

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
	if (HSM_DEDITHER_MODE < 2.5 || HSM_DEDITHER_MODE > 4.5)
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}

   FragColor = vec4(GAMMA_IN(texture(Source, vTexCoord).rgb), 1.0);
}

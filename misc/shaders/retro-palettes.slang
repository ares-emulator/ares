#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float pal;
} params;

#pragma parameter pal "color palette colors: (1+x)^3" 1.0 1.0 15.0 1.0
#pragma parameter something "1: ZX Spectrum, 2: Amstrad, 3: EGA, 7:Genesis, 15:Amiga" 0.0 0.0 0.0 0.0
#define pal params.pal

#define SourceSize params.SourceSize
#define OutputSize params.OutputSize
#define OriginalSize params.OriginalSize

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
   vTexCoord = TexCoord*1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{	
	vec3 res = texture(Source,vTexCoord).rgb;

  	res= round(res*pal)/pal;
   FragColor = vec4(res,1.0);
}
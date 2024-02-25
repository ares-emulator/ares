#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float height;
	float width;
	float location;
} params;

#pragma parameter height "CocktailTable Image Height" 1.0 -3.0 3.0 0.01
#pragma parameter width "CocktailTable Image Width" 1.0 -3.0 3.0 0.05
#pragma parameter location "CocktailTable Image Separation" 0.0001 -2.0 2.0 0.001

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vTexCoord1;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord  = ((TexCoord.xy - vec2(0.5 - (params.location), 0.5)) * mat2x2(0.0,  1.0001 * (params.width), -2.0001 * (params.height), 0.0)) + vec2(0.5, 0. );
   vTexCoord1 = ((TexCoord.xy - vec2(0.5 + (params.location), 0.5)) * mat2x2(0.0, -1.0001 * (params.width),  2.0001 * (params.height), 0.0)) + vec2(0.5, 0. );
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vTexCoord1;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   FragColor = vec4(texture(Source, vTexCoord).rgb, 1.0) + vec4(texture(Source, vTexCoord1).rgb, 1.0);
}
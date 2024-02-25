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

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 blurCoordinates[5];

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
	blurCoordinates[0] = vTexCoord.xy;
	blurCoordinates[1] = vTexCoord.xy + params.OutputSize.zw * vec2(0.0,1.407333);
	blurCoordinates[2] = vTexCoord.xy - params.OutputSize.zw * vec2(0.0,1.407333);
	blurCoordinates[3] = vTexCoord.xy + params.OutputSize.zw * vec2(0.0,3.294215);
	blurCoordinates[4] = vTexCoord.xy - params.OutputSize.zw * vec2(0.0,3.294215);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 blurCoordinates[5];
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	vec4 sum = vec4(0.0);
	sum += texture(Source, blurCoordinates[0]) * 0.204164;
	sum += texture(Source, blurCoordinates[1]) * 0.304005;
	sum += texture(Source, blurCoordinates[2]) * 0.304005;
	sum += texture(Source, blurCoordinates[3]) * 0.093913;
	sum += texture(Source, blurCoordinates[4]) * 0.093913;
	FragColor = sum;
}

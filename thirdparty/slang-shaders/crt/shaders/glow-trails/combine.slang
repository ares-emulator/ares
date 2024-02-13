#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float screen_combine;
	float haze_strength;
} params;

#pragma parameter screen_combine "Screen Combine" 0.0 0.0 1.0 1.0
#pragma parameter haze_strength "Haze Strength" 0.5 0.0 1.0 0.05

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
layout(set = 0, binding = 3) uniform sampler2D PASS1;

void main()
{
vec4 blurred = pow(texture(Source, vTexCoord), vec4(2.2));
vec4 unblurred = pow(texture(PASS1, vTexCoord), vec4(2.2));
vec4 dark = vec4(pow(mix(unblurred, blurred, params.haze_strength), vec4(1.0 / 2.2)));//vec4(pow((unblurred + blurred) / 2.0, vec4(1.0 / 2.2)));
vec4 bright = vec4(pow(vec4(1.0) - (vec4(1.0) - unblurred) * (vec4(1.0) - blurred), vec4(1.0 / 2.2)));
   FragColor = (params.screen_combine < 0.5) ? dark : bright;
}
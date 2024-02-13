#version 450

// Super-basic scanline shader
// (looks bad at non-integer scales)
// by hunterk
// license: public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float THICKNESS;
	float DARKNESS;
	float BRIGHTBOOST;
} params;

#pragma parameter THICKNESS "Scanline Thickness" 2.0 1.0 12.0 1.0
#pragma parameter DARKNESS "Scanline Darkness" 0.50 0.0 1.0 0.05
#pragma parameter BRIGHTBOOST "Scanline Boost Bright" 1.1 1.0 1.2 0.1

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

vec3 RGBtoYIQ(vec3 RGB){
	const mat3 yiqmat = mat3(
		0.2989, 0.5870, 0.1140,
		0.5959, -0.2744, -0.3216,
		0.2115, -0.5229, 0.3114);
	return RGB * yiqmat;
}

vec3 YIQtoRGB(vec3 YIQ){
	const mat3 rgbmat = mat3(
		1.0, 0.956, 0.6210,
		1.0, -0.2720, -0.6474,
		1.0, -1.1060, 1.7046);
	return YIQ * rgbmat;
}

void main()
{
	float lines = fract(vTexCoord.y * params.SourceSize.y);
	float scale_factor = floor((params.OutputSize.y / params.SourceSize.y) + 0.4999);
	vec4 screen = texture(Source, vTexCoord);
	screen.rgb = RGBtoYIQ(screen.rgb);
	screen.r *= params.BRIGHTBOOST;
	screen.rgb = YIQtoRGB(screen.rgb);

	FragColor = (lines > (1.0 / scale_factor * params.THICKNESS)) ? screen : screen * vec4(1.0 - params.DARKNESS);
}